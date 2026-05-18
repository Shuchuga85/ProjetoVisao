# Guia de Estudo — Pipeline de Detecção de Laranjas

Este guia explica cada algoritmo principal usado no programa, pela ordem em que são chamados em cada frame.
Inclui pseudo-código e exemplos para que consigam explicar a lógica sem precisar de memorizar o código C.

---

## Pipeline Geral (uma frame)

```
Frame BGR
  │
  ├─ bgr_to_rgb ──► imagem rgb
  │
  ├─ rgb_to_hsv ──► imagem hsv
  │
  ├─ hsv_segmentation ──► máscara binária (branco = pixels laranja)
  │
  ├─ binary_close ──► máscara binária limpa (buracos preenchidos, ruído removido)
  │
  ├─ rgb_to_gray + mask_crop ──► escala de cinzentos, só dentro das regiões laranja
  │
  ├─ gray_local_mean_detect_fast ──► binário de manchas (branco = manchas escuras)
  │
  ├─ blob_labelling + blob_info ──► array de blobs com geometria básica
  │
  ├─ orange_classify (1ª passagem) ──► preenche diameter_mm, deformation, calibre
  │
  ├─ blob_filter_deformation ──► mantém apenas laranjas válidas
  │
  ├─ blob_count_dark_spots ──► preenche spot_ratio por blob
  │
  ├─ orange_classify (2ª passagem) ──► finaliza categoria com dados das manchas
  │
  └─ count_new_oranges ──► acumula total de laranjas únicas vistas
```

---

## 1. RGB → HSV  (`vc_rgb_to_hsv`)

### Porquê?
O modelo RGB mistura cor e brilho — uma laranja clara e uma laranja escura parecem muito diferentes em RGB.
O HSV separa-os:
- **H (Hue / Matiz)** — a cor em si (0°–360°, guardado como 0–255)
- **S (Saturation / Saturação)** — quão viva/pura é a cor (0–100%, guardado como 0–255)
- **V (Value / Brilho)** — quão brilhante é (0–100%, guardado como 0–255)

### Fórmula
```
r, g, b  →  normalizar para [0, 1]

max = max(r, g, b)
min = min(r, g, b)
delta = max - min

V = max

S = delta / max        (0 se max == 0)

H = depende de qual canal é o máximo:
    se max == r:  H = 60 × (g - b) / delta
    se max == g:  H = 120 + 60 × (b - r) / delta
    se max == b:  H = 240 + 60 × (r - g) / delta
    se H < 0: H += 360
```

### Exemplo concreto
| Pixel | R | G | B | → | H (°) | S (%) | V (%) |
|-------|---|---|---|---|--------|--------|--------|
| Laranja clara | 255 | 140 | 0 | | ~33° | 100% | 100% |
| Laranja escura | 130 | 70 | 0 | | ~32° | 100% | 51% |
| Parede branca | 240 | 240 | 240 | | 0° | 0% | 94% |

Ambas as laranjas têm quase o mesmo Hue — por isso o HSV é ideal para detecção por cor.

---

## 2. Segmentação HSV  (`vc_hsv_segmentation`)

### O que faz
Percorre cada pixel da imagem HSV. Se o H, S e V do pixel estiverem todos dentro dos intervalos definidos, coloca **branco (255)**. Caso contrário, **preto (0)**.

O resultado é uma **imagem binária** — branco onde foi encontrada cor laranja.

### Parâmetros usados
```
H: 15° – 35°   (banda de matiz laranja)
S: 33% – 100%  (não demasiado cinzento/desbotado)
V: 25% – 100%  (não demasiado escuro / sombra)
```

### Pseudo-código
```
para cada pixel (x, y):
    h = pixel.H
    s = pixel.S
    v = pixel.V

    se h está em [min_H, max_H]
    E s está em [min_S, max_S]
    E v está em [min_V, max_V]:
        saída[x,y] = BRANCO (255)
    senão:
        saída[x,y] = PRETO (0)
```

### Exemplo visual
```
Frame original:          Após segmentação:
[laranja] [parede] [tapete]   [BRANCO] [PRETO] [PRETO]
[laranja] [laranja][tapete]   [BRANCO] [BRANCO][PRETO]
[sombra ] [laranja][tapete]   [PRETO ] [BRANCO][PRETO]
```
O pixel de sombra é excluído porque o seu V (brilho) é demasiado baixo.

> **Nota:** Os valores são internamente reescalados de graus/percentagem para [0–255] antes da comparação, porque a imagem HSV guarda valores como bytes.

---

## 3. Fecho Morfológico  (`vc_binary_close`)

### O que faz
Fecha pequenos **buracos e falhas** dentro das regiões brancas da máscara binária.
É uma operação em dois passos:
1. **Dilatar** — expandir cada região branca para fora (preenche falhas entre pixels brancos próximos)
2. **Erodir** — encolher cada região branca para dentro pela mesma quantidade (restaura o tamanho original, mas as falhas ficam preenchidas)

### Kernel usado: 3×3
```
Dilatar: se ALGUM pixel na janela 3×3 for branco → saída = branco
Erodir:  se TODOS os pixels na janela 3×3 forem brancos → saída = branco
```

### Exemplo visual (simplificado em 1D)
```
Antes do close:   0 0 1 1 0 1 1 0 0   (dois grupos com uma falha)
Após dilatar:     0 1 1 1 1 1 1 1 0   (falha preenchida, bordas expandidas)
Após erodir:      0 0 1 1 1 1 1 0 0   (bordas encolhidas, falha continua preenchida)
```

### Porquê é importante
Sem o close, uma laranja parcialmente ensombrada pode aparecer como dois blobs separados em vez de um. O close une-os numa única região conectada.

### Implementação rápida (prefix sums)
Em vez de deslizar um kernel sobre cada pixel individualmente (lento), o código usa um **array de somas acumuladas** (também chamado imagem integral):
- **Passagem horizontal:** constrói uma soma acumulada por linha, depois responde "quantos pixels brancos existem nas colunas [x-k, x+k]?" em O(1) por pixel
- **Passagem vertical:** a mesma ideia por coluna

Isto torna o algoritmo O(largura × altura) em vez de O(largura × altura × kernel²).

---

## 4. Máscara de Recorte  (`vc_mask_crop`)

### O que faz
Muito simples: copia a imagem em escala de cinzentos mas coloca **0 (preto) em cada pixel onde a máscara binária é preta**.

O resultado é uma imagem em escala de cinzentos que só tem valores válidos dentro das regiões laranja — tudo o resto está a zero.

### Pseudo-código
```
para cada pixel (x, y):
    se máscara[x, y] == BRANCO:
        saída[x, y] = cinzento[x, y]   // manter o valor real
    senão:
        saída[x, y] = 0                 // apagar o fundo
```

### Porquê é necessário
O detector de manchas escuras corre sobre esta imagem recortada. Sem o recorte, o tapete e o fundo pretos da passadeira seriam incorretamente detetados como "manchas escuras" na laranja.

---

## 5. Detecção de Manchas Escuras  (`vc_gray_local_mean_detect_fast`)

### Objetivo
Encontrar pixels dentro de uma laranja que sejam **mais escuros do que a sua vizinhança** — estas são nódoas, podridão ou defeitos.

### Ideia principal: comparação com a média local
Para cada pixel, calcula-se a **média de brilho da sua vizinhança** (janela de 31×31).
Se o pixel for nitidamente mais escuro que a sua média local, é uma mancha.

```
residual = média_local - valor_do_pixel

se residual > threshold (8):
    marcar como MANCHA (branco)
senão:
    não é mancha (preto)
```

Um pixel que está 8+ níveis de cinzento abaixo dos seus vizinhos é considerado uma mancha.

### Exemplo
```
Pixels da vizinhança (valores de brilho):
  150 148 152 149 151
  147 153  40 150 148   ← pixel central = 40 (muito mais escuro)
  151 149 150 152 147
  ...

média_local ≈ 148
residual = 148 - 40 = 108  →  108 > 8  →  MANCHA detetada
```

```
Outro pixel:
  pixel central = 143, média_local = 148
  residual = 5  →  5 < 8  →  NÃO é mancha (apenas sombra normal)
```

### SAT (Summed Area Table) — porquê é rápido
Calcular a soma numa janela 31×31 de forma direta implica ler 961 pixels por pixel.
Para uma imagem 1280×720 são 886 milhões de leituras — demasiado lento para tempo real.

Uma SAT é uma tabela pré-calculada onde cada célula guarda a **soma de todos os pixels acima e à esquerda**:

```
SAT[y][x] = pixel[y][x]
           + SAT[y-1][x]
           + SAT[y][x-1]
           - SAT[y-1][x-1]   (subtrair canto contado duas vezes)
```

Uma vez construída, a soma de qualquer rectângulo é apenas **4 acessos à tabela**:
```
soma(região) = SAT[y1][x1] - SAT[y0-1][x1] - SAT[y1][x0-1] + SAT[y0-1][x0-1]
```

```
   +------------------+
   |         |        |
   |    D    |   C    |
   |         |        |
   +---------+--------+
   |         |        |
   |    B    | região |
   |         |        |
   +---------+--------+

soma(região) = A - B - C + D     (onde A, B, C, D são valores dos cantos da SAT)
```

Isto mantém o algoritmo em O(largura × altura) independentemente do tamanho do kernel.

**Tanto a soma como a contagem de pixels** são guardadas (pixels com valor 0 são fundo e excluídos da média).

---

## 6. Etiquetagem de Blobs  (`vc_binary_blob_labelling`)

### O que faz
Recebe a máscara binária (branco = região laranja) e atribui um **número inteiro único** a cada grupo conectado de pixels brancos.

A imagem de saída é uma imagem de etiquetas onde:
- pixels pertencentes à laranja 1 → valor 1
- pixels pertencentes à laranja 2 → valor 2
- fundo → 0

### Algoritmo: componentes conexas em dois passos
**Passo 1 — varredura da esquerda para a direita, de cima para baixo:**
```
para cada pixel branco:
    olhar para o vizinho à esquerda e o vizinho de cima
    se ambos são fundo → atribuir uma nova etiqueta
    se um tem etiqueta → copiar essa etiqueta
    se ambos têm etiquetas diferentes → atribuir a menor,
                                        registar que as duas são equivalentes
```

**Passo 2 — resolver equivalências:**
```
percorrer todos os pixels novamente
substituir cada etiqueta pela raiz do seu grupo de equivalência
```

### Resultado
```
Máscara binária:      Imagem de etiquetas:
0 1 1 0 0 1 1         0 1 1 0 0 2 2
0 1 1 0 0 1 1         0 1 1 0 0 2 2
0 0 0 0 0 0 0         0 0 0 0 0 0 0
0 0 1 1 1 0 0         0 0 3 3 3 0 0
```
Três blobs separados → etiquetas 1, 2, 3.

---

## 7. Informação dos Blobs  (`vc_binary_blob_info`)

### O que faz
Para cada blob etiquetado, percorre a imagem de etiquetas uma vez e calcula:

| Campo | Como é calculado |
|-------|-----------------|
| `area` | contagem de pixels com esta etiqueta |
| `xc, yc` (centróide) | média do x e média do y de todos esses pixels |
| `x, y` (canto superior esquerdo da bounding box) | min x, min y encontrados |
| `width, height` (bounding box) | max_x − min_x + 1, max_y − min_y + 1 |
| `perimeter` | pixels que têm pelo menos um vizinho em 4-conectividade com etiqueta diferente |

### Detecção do perímetro
Um pixel está no perímetro se algum dos seus 4 vizinhos diretos (cima/baixo/esquerda/direita) tiver uma etiqueta diferente:
```
pixel em (x,y) é pixel de borda se:
    etiqueta[y][x-1] ≠ etiqueta[y][x]   (esquerda)
 OU etiqueta[y][x+1] ≠ etiqueta[y][x]   (direita)
 OU etiqueta[y-1][x] ≠ etiqueta[y][x]   (cima)
 OU etiqueta[y+1][x] ≠ etiqueta[y][x]   (baixo)
```

### Exemplo de valores para um blob
```
area     = 95.000 px
centróide = (640, 360)
bbox     = x=490, y=210, largura=300, altura=280
perimeter = 940 px
```

---

## 8. Classificação da Laranja  (`vc_orange_classify`)

Esta função preenche os restantes campos do OVC: `diameter_mm`, `calibre`, `deformation` e `category`.
É chamada **duas vezes por frame**: uma antes do filtro (para obter medidas para o filtro), e outra após a contagem de manchas (para obter a categoria final).

### Conversão de escala
```
280 pixels = 55 mm   →   1 pixel = 55/280 = 0,196 mm
```

### 8a. Diâmetro
```
diameter_px = (blob.largura + blob.altura) / 2    // média dos dois eixos
diameter_mm = diameter_px × (55 / 280)
```

Exemplo: blob tem 300×280 px → diameter_px = 290 → diameter_mm = 290 × 0,196 = **56,9 mm**

### 8b. Calibre (tabela de tamanhos EEC 379/71)
O calibre é um número de 0 a 13 que indica em que banda de tamanho a laranja se enquadra.
O código verifica o diâmetro medido contra a tabela oficial:

| Calibre | Mínimo (mm) | Máximo (mm) |
|---------|-------------|-------------|
| 0       | 100         | ∞           |
| 1       | 87          | 100         |
| 2       | 84          | 96          |
| ...     | ...         | ...         |
| 9       | 62          | 70          |
| 13      | 53          | 60          |

```
para cada banda de calibre [min, max]:
    se diameter_mm está dentro de [min, max]:
        calcular distância ao centro da banda
        manter a banda cujo centro esteja mais próximo
```

### 8c. Deformação
Mede o quão afastada está a forma real da laranja de um círculo perfeito.

```
lado_max    = max(blob.largura, blob.altura)     // eixo mais longo em pixels
área_ideal  = π × (lado_max / 2)²                // área de um círculo perfeito desse tamanho
área_real   = blob.area

deformation = |área_real - área_ideal| / área_ideal × 100  (%)
```

Exemplo:
```
blob.largura=300, blob.altura=280  →  lado_max=300
área_ideal = π × 150² = 70.686 px
área_real = 68.000 px
deformation = |68000 - 70686| / 70686 × 100 = 3,8%  → forma Extra
```

Um círculo perfeito daria 0%. Uma laranja achatada ou alongada dá uma percentagem mais alta.

### 8d. Categoria (parte da forma)
```
se deformation < 5%   → Extra
se deformation < 10%  → Classe I
se deformation < 20%  → Classe II
senão                  → Classe III
```

Após a contagem de manchas, calcula-se também a categoria das manchas:
```
se spot_ratio < 0,5%  → Extra
se spot_ratio < 1,0%  → Classe I
se spot_ratio < 2,0%  → Classe II
senão                  → Classe III
```

A **categoria final é a pior das duas** (forma vs. manchas).

---

## 9. Filtro por Deformação  (`vc_blob_filter_deformation`)

### O que faz
Após a classificação, remove blobs que não podem ser laranjas válidas:

```
para cada blob:
    se deformation > 20%  →  descartar  (demasiado irregular, não é uma laranja)
    se diameter_mm < 53   →  descartar  (abaixo do tamanho mínimo EEC)
    senão                 →  manter
```

Constrói um novo array apenas com os blobs aceites.

### Porquê estes thresholds?
- `diameter < 53 mm` — O regulamento EEC 379/71 define 53 mm como o tamanho mínimo absoluto de laranja comercializável. Qualquer coisa mais pequena não é uma laranja válida para venda.
- `deformation > 20%` — Corresponde à Classe III, a última categoria aceite. Qualquer coisa além disso é demasiado irregular para classificar de forma significativa (provavelmente ruído, blob parcial ou objeto que não é uma laranja).

### Exemplo
```
Blob A: diâmetro=65mm, deformation=7%   → MANTER   (forma Classe I)
Blob B: diâmetro=48mm, deformation=5%   → DESCARTAR (demasiado pequeno)
Blob C: diâmetro=70mm, deformation=28%  → DESCARTAR (demasiado deformado)
Blob D: diâmetro=59mm, deformation=15%  → MANTER   (forma Classe II)
```

---

## 10. Contagem de Manchas Escuras  (`vc_blob_count_dark_spots`)

### O que faz
Conta quantos **pixels de mancha** (da imagem binária de manchas) estão dentro de cada blob de laranja.
Usa a imagem de etiquetas para saber a que blob pertence cada pixel.

### Pseudo-código
```
// Construir tabela de consulta: número de etiqueta → índice do blob
para cada blob i:
    idx[blob[i].etiqueta] = i

// Contar manchas por blob
para cada pixel (x, y):
    se spot_binary[x, y] == BRANCO:              // é uma mancha escura
        etq = imagem_etiquetas[x, y]             // a que laranja pertence?
        blobs[idx[etq]].dark_spot_area++         // adicionar ao contador dessa laranja

// Calcular rácio
para cada blob i:
    total = blob.area + blob.dark_spot_area
    blob.spot_ratio = dark_spot_area / total × 100  (%)
```

### Exemplo
```
Área do blob laranja = 95.000 px
Pixels de mancha escura dentro dele = 720 px

total = 95.000 + 720 = 95.720
spot_ratio = 720 / 95.720 × 100 = 0,75%

→ 0,5% < 0,75% < 1,0%  →  manchas elevam categoria para Classe I
```

---

## 11. Contagem de Novas Laranjas  (`vc_count_new_oranges`)

### Objetivo
Acompanhar quantas laranjas **únicas** apareceram no vídeo, sem contar a mesma laranja várias vezes enquanto se move pela frame.

### Como funciona
Em cada frame, a função compara os centróides dos blobs atuais com os centróides da **frame anterior**.

```
para cada blob na frame atual:
    é_nova = verdadeiro
    para cada centróide na frame anterior:
        distância = sqrt((xc_atual - xc_anterior)² + (yc_atual - yc_anterior)²)
        se distância < 80 pixels:
            é_nova = falso   // mesma laranja, apenas moveu-se um pouco
            parar

    se é_nova:
        total++

// Guardar centróides atuais como "anteriores" para a próxima frame
```

### O threshold de 80 pixels
Uma laranja a velocidade normal de passadeira move menos de 80 pixels entre duas frames consecutivas. Por isso, se um centróide estiver a menos de 80 px de um na frame anterior, é a mesma laranja.

### Exemplo
```
Centróides na frame 100: (200, 360), (500, 360), (800, 360)
Centróides na frame 101: (190, 360), (490, 360), (795, 360), (1050, 360)

Laranja em (1050, 360): nenhum centróide anterior a menos de 80px → NOVA → total++
As outras: associadas à frame 100 → não são novas
```

---

## 12. Verificação de Uniformidade (em `main.cpp`)

Não é uma função separada, mas vale a pena perceber para a apresentação.

### Tracking global de mínimo e máximo
Em cada frame, após o filtro:
```
para cada blob em filtered_blobs:
    se blob.diameter_mm > global_max → atualizar global_max
    se blob.diameter_mm < global_min → atualizar global_min
```
Como o mínimo e o máximo naturalmente se fixam nos valores extremos reais, medir a mesma laranja várias vezes ao longo das frames não distorce o resultado.

### Regra de uniformidade EEC 379/71
O regulamento limita o quão diferentes podem ser a maior e a menor laranja num lote:

| Situação | Diferença máxima permitida |
|----------|---------------------------|
| Todas as laranjas ≥ 73 mm (calibres 1–8) | 10 mm |
| Alguma laranja < 73 mm (calibres 9–13) | 7 mm |

```
diferença = diâmetro_global_max - diâmetro_global_min

se global_min >= 73mm:
    tolerância = 10mm
senão:
    tolerância = 7mm

se diferença <= tolerância → CONFORME
senão                      → NÃO CONFORME
```

### Exemplo
```
Menor laranja vista: 61,2 mm  →  abaixo de 73mm  →  tolerância = 7mm
Maior laranja vista: 70,8 mm

diferença = 70,8 - 61,2 = 9,6 mm
9,6 > 7,0  →  NÃO CONFORME
```

---

## Referência Rápida — Constantes em `vc.h`

| Constante | Valor | Significado |
|-----------|-------|-------------|
| `VC_PX_TO_MM` | 55/280 ≈ 0,196 | escala pixel para mm |
| `VC_ORANGE_H_MIN/MAX` | 15 – 35 | intervalo de matiz laranja (graus) |
| `VC_ORANGE_S_MIN/MAX` | 33 – 100 | intervalo de saturação laranja (%) |
| `VC_ORANGE_V_MIN/MAX` | 25 – 100 | intervalo de brilho laranja (%) |
| `VC_SPOT_KERNEL_LOCAL` | 31 | janela de vizinhança para detecção de manchas |
| `VC_SPOT_THRESHOLD` | 8 | quantos níveis abaixo dos vizinhos = mancha |
| `VC_DEFORM_EXTRA/I/II` | 5 / 10 / 20% | limites de categoria por deformação |
| `VC_SPOT_EXTRA/I/II` | 0,5 / 1,0 / 2,0% | limites de categoria por rácio de manchas |
| `VC_MIN_DIAMETER_MM` | 53 mm | tamanho mínimo válido de laranja |
| `VC_FILTER_DEFORM_MAX` | 20% | deformação máxima antes do blob ser descartado |
| `VC_DIST_MAX` | 80 px | movimento máximo entre frames (tracking) |
| `VC_UNIFORM_TOL_LARGE` | 10 mm | tolerância de uniformidade para calibres grandes |
| `VC_UNIFORM_TOL_SMALL` | 7 mm | tolerância de uniformidade para calibres pequenos |
| `VC_UNIFORM_BOUNDARY_MM` | 73 mm | limite entre grupos de calibres grande/pequeno |
