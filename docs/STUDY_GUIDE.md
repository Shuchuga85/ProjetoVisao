# Study Guide — Orange Detection Pipeline

This guide explains every main algorithm used in the program, in the order they are called each frame.
Pseudo-code and examples are included so you can explain the logic out loud without needing to memorise the C code.

---

## Overall Pipeline (one frame)

```
BGR frame
  │
  ├─ bgr_to_rgb ──► rgb image
  │
  ├─ rgb_to_hsv ──► hsv image
  │
  ├─ hsv_segmentation ──► binary mask (white = orange pixels)
  │
  ├─ binary_close ──► cleaned binary mask (holes filled, noise removed)
  │
  ├─ rgb_to_gray + mask_crop ──► grayscale, only inside orange regions
  │
  ├─ gray_local_mean_detect_fast ──► spot binary (white = dark spots)
  │
  ├─ blob_labelling + blob_info ──► array of blobs with basic geometry
  │
  ├─ orange_classify (1st pass) ──► fills diameter_mm, deformation, calibre
  │
  ├─ blob_filter_deformation ──► keeps only valid oranges
  │
  ├─ blob_count_dark_spots ──► fills spot_ratio per blob
  │
  ├─ orange_classify (2nd pass) ──► finalises category with spot data
  │
  └─ count_new_oranges ──► accumulates total unique oranges seen
```

---

## 1. RGB → HSV  (`vc_rgb_to_hsv`)

### Why?
RGB mixes colour and brightness together — a bright orange and a dark orange look very different in RGB.
HSV separates them:
- **H (Hue)** — the actual colour (0°–360°, stored as 0–255)
- **S (Saturation)** — how vivid/pure the colour is (0–100%, stored as 0–255)
- **V (Value)** — how bright it is (0–100%, stored as 0–255)

### Formula
```
r, g, b  →  normalise to [0, 1]

max = max(r, g, b)
min = min(r, g, b)
delta = max - min

V = max

S = delta / max        (0 if max == 0)

H = depends on which channel is max:
    if max == r:  H = 60 * (g - b) / delta
    if max == g:  H = 120 + 60 * (b - r) / delta
    if max == b:  H = 240 + 60 * (r - g) / delta
    if H < 0: H += 360
```

### Concrete example
| Pixel | R | G | B | → | H (°) | S (%) | V (%) |
|-------|---|---|---|---|--------|--------|--------|
| Bright orange | 255 | 140 | 0 | | ~33° | 100% | 100% |
| Dark orange   | 130 | 70  | 0 | | ~32° | 100% | 51%  |
| White wall    | 240 | 240 | 240 | | 0° | 0% | 94% |

Both oranges have almost the same Hue, which is why HSV is ideal for colour-based detection.

---

## 2. HSV Segmentation  (`vc_hsv_segmentation`)

### What it does
Goes through every pixel of the HSV image. If the pixel's H, S, and V all fall inside the given ranges, it outputs **white (255)**. Otherwise **black (0)**.

The result is a **binary image** — white where orange colour was found.

### Parameters used
```
H: 15° – 35°   (orange hue band)
S: 33% – 100%  (not too grey/washed out)
V: 25% – 100%  (not too dark / shadow)
```

### Pseudo-code
```
for every pixel (x, y):
    h = pixel.H
    s = pixel.S
    v = pixel.V

    if h is in [min_H, max_H]
    AND s is in [min_S, max_S]
    AND v is in [min_V, max_V]:
        output[x,y] = WHITE (255)
    else:
        output[x,y] = BLACK (0)
```

### Visual example
```
Original frame:          After segmentation:
[orange] [wall] [belt]   [WHITE ] [BLACK] [BLACK]
[orange] [orange][belt]  [WHITE ] [WHITE] [BLACK]
[shadow] [orange][belt]  [BLACK ] [WHITE] [BLACK]
```
The shadow pixel is excluded because its V (brightness) is too low.

> **Note:** The values are internally rescaled from degrees/percent to [0–255] before comparison, because the HSV image stores values as bytes.

---

## 3. Morphological Close  (`vc_binary_close`)

### What it does
Closes small **holes and gaps** inside the white regions of the binary mask.
It is a two-step operation:
1. **Dilate** — expand every white region outward (fills gaps between nearby white pixels)
2. **Erode** — shrink every white region inward by the same amount (restores original size but gaps are now gone)

### Kernel used: 3×3
```
Dilate: if ANY pixel in the 3×3 window is white → output = white
Erode:  if ALL pixels in the 3×3 window are white → output = white
```

### Visual example (1D simplified)
```
Before close:  0 0 1 1 0 1 1 0 0   (two groups with a gap)
After dilate:  0 1 1 1 1 1 1 1 0   (gap filled, edges expanded)
After erode:   0 0 1 1 1 1 1 0 0   (edges shrunk back, gap stays filled)
```

### Why it matters
Without closing, a partially shadowed orange can appear as two separate blobs instead of one. Closing merges them into a single connected region.

### Fast implementation (prefix sums)
Instead of sliding a kernel over every pixel one by one (slow), the code uses a **prefix sum array** (also called an integral image):
- **Horizontal pass:** build a running sum per row, then answer "how many white pixels are in columns [x-k, x+k]?" in O(1) per pixel
- **Vertical pass:** same idea per column

This makes it O(width × height) instead of O(width × height × kernel²).

---

## 4. Mask Crop  (`vc_mask_crop`)

### What it does
Very simple: copies the grayscale image but sets every pixel to **0 (black) where the binary mask is black**.

The result is a grayscale image that only has valid pixel values inside the orange regions — everything else is zeroed out.

### Pseudo-code
```
for every pixel (x, y):
    if mask[x, y] == WHITE:
        output[x, y] = gray[x, y]   // keep the real value
    else:
        output[x, y] = 0             // erase background
```

### Why it's needed
The dark-spot detector runs on this cropped image. Without cropping, the black conveyor belt and background would be incorrectly detected as "dark spots" on the orange.

---

## 5. Dark Spot Detection  (`vc_gray_local_mean_detect_fast`)

### Goal
Find pixels inside an orange that are **darker than their surroundings** — these are bruises, rot, or blemishes.

### Core idea: local mean comparison
For each pixel, compute the **average brightness of its neighbourhood** (a 31×31 window).
If the pixel is noticeably darker than its local average, it is a dark spot.

```
residual = local_mean - pixel_value

if residual > threshold (8):
    mark as SPOT (white)
else:
    not a spot (black)
```

A pixel that is 8+ grey levels below its neighbours is considered a spot.

### Example
```
Neighbourhood pixels (brightness values):
  150 148 152 149 151
  147 153  40 150 148   ← centre pixel = 40 (much darker)
  151 149 150 152 147
  ...

local_mean ≈ 148
residual = 148 - 40 = 108  →  108 > 8  →  SPOT detected
```

```
Another pixel:
  centre pixel = 143, local_mean = 148
  residual = 5  →  5 < 8  →  NOT a spot (just normal shading)
```

### SAT (Summed Area Table) — why it's fast
Computing the sum over a 31×31 window naively means reading 961 pixels per pixel.
For a 1280×720 image that's 886 million reads — too slow for real-time.

A SAT is a pre-computed table where each cell stores the **sum of all pixels above and to the left**:

```
SAT[y][x] = pixel[y][x]
           + SAT[y-1][x]
           + SAT[y][x-1]
           - SAT[y-1][x-1]   (subtract corner counted twice)
```

Once built, the sum of any rectangle is just **4 table lookups**:
```
sum(region) = SAT[y1][x1] - SAT[y0-1][x1] - SAT[y1][x0-1] + SAT[y0-1][x0-1]
```

```
   +------------------+
   |         |        |
   |    D    |   C    |
   |         |        |
   +---------+--------+
   |         |        |
   |    B    | region |
   |         |        |
   +---------+--------+

sum(region) = A - B - C + D     (where A, B, C, D are SAT corner values)
```

This keeps the algorithm at O(width × height) regardless of kernel size.

**Both sum and pixel count** are tracked (pixels with value 0 are background and excluded from the mean).

---

## 6. Blob Labelling  (`vc_binary_blob_labelling`)

### What it does
Takes the binary mask (white = orange region) and assigns a unique **integer label** to each connected group of white pixels.

The output is a label image where:
- pixels belonging to orange 1 → value 1
- pixels belonging to orange 2 → value 2
- background → 0

### Algorithm: two-pass connected components
**Pass 1 — scan left-to-right, top-to-bottom:**
```
for each white pixel:
    look at left neighbour and top neighbour
    if both are background → assign a new label
    if one has a label → copy that label
    if both have different labels → assign the smaller one,
                                    record that the two labels are equivalent
```

**Pass 2 — resolve equivalences:**
```
go through all pixels again
replace each label with the root of its equivalence group
```

### Result
```
Binary mask:          Label image:
0 1 1 0 0 1 1         0 1 1 0 0 2 2
0 1 1 0 0 1 1         0 1 1 0 0 2 2
0 0 0 0 0 0 0         0 0 0 0 0 0 0
0 0 1 1 1 0 0         0 0 3 3 3 0 0
```
Three separate orange blobs → labels 1, 2, 3.

---

## 7. Blob Info  (`vc_binary_blob_info`)

### What it does
For each labelled blob, scans the label image once and computes:

| Field | How it's computed |
|-------|-------------------|
| `area` | count of pixels with this label |
| `xc, yc` (centroid) | average x and average y of all those pixels |
| `x, y` (bounding box top-left) | min x, min y seen |
| `width, height` (bounding box) | max_x − min_x + 1, max_y − min_y + 1 |
| `perimeter` | count pixels that have at least one 4-connected neighbour with a different label |

### Perimeter detection
A pixel is on the perimeter if any of its 4 direct neighbours (up/down/left/right) has a different label:
```
pixel at (x,y) is a border pixel if:
    label[y][x-1] ≠ label[y][x]   (left)
 OR label[y][x+1] ≠ label[y][x]   (right)
 OR label[y-1][x] ≠ label[y][x]   (above)
 OR label[y+1][x] ≠ label[y][x]   (below)
```

### Example values for one orange blob
```
area     = 95,000 px
centroid = (640, 360)
bbox     = x=490, y=210, width=300, height=280
perimeter = 940 px
```

---

## 8. Orange Classify  (`vc_orange_classify`)

This function fills the remaining OVC fields: `diameter_mm`, `calibre`, `deformation`, and `category`.
It is called **twice per frame**: once before filtering (to get measurements for the filter), and once after spot counting (to get the final category).

### Scale conversion
```
280 pixels = 55 mm   →   1 pixel = 55/280 = 0.196 mm
```

### 8a. Diameter
```
diameter_px = (blob.width + blob.height) / 2    // average of both axes
diameter_mm = diameter_px × (55 / 280)
```

Example: blob is 300×280 px → diameter_px = 290 → diameter_mm = 290 × 0.196 = **56.9 mm**

### 8b. Calibre (EEC 379/71 size table)
The calibre is a number 0–13 that says which size band the orange falls into.
The code checks the measured diameter against the official table:

| Calibre | Min (mm) | Max (mm) |
|---------|----------|----------|
| 0       | 100      | ∞        |
| 1       | 87       | 100      |
| 2       | 84       | 96       |
| ...     | ...      | ...      |
| 9       | 62       | 70       |
| 13      | 53       | 60       |

```
for each calibre band [lo, hi]:
    if diameter_mm is inside [lo, hi]:
        compute distance to the band centre
        keep the band whose centre is closest
```

### 8c. Deformation
Measures how far the orange's actual shape is from a perfect circle.

```
max_side    = max(blob.width, blob.height)     // longest axis in pixels
ideal_area  = π × (max_side / 2)²              // area of a perfect circle that size
actual_area = blob.area

deformation = |actual_area - ideal_area| / ideal_area × 100  (%)
```

Example:
```
blob.width=300, blob.height=280  →  max_side=300
ideal_area = π × 150² = 70,686 px
actual_area = 68,000 px
deformation = |68000 - 70686| / 70686 × 100 = 3.8%  → Extra shape
```

A perfect circle would give 0%. A squashed or elongated orange gives a higher %.

### 8d. Category (shape part)
```
if deformation < 5%   → Extra
if deformation < 10%  → Class I
if deformation < 20%  → Class II
else                   → Class III
```

After spot counting, the spot category is also computed:
```
if spot_ratio < 0.5%  → Extra
if spot_ratio < 1.0%  → Class I
if spot_ratio < 2.0%  → Class II
else                   → Class III
```

The **final category is the worst of the two** (shape vs. spots).

---

## 9. Blob Filter Deformation  (`vc_blob_filter_deformation`)

### What it does
After classifying, removes blobs that cannot be valid oranges:

```
for each blob:
    if deformation > 20%  →  discard  (too irregular, not an orange)
    if diameter_mm < 53   →  discard  (below EEC minimum size)
    else                  →  keep
```

Builds a new array with only the accepted blobs.

### Why these thresholds?
- `diameter < 53 mm` — The EEC 379/71 regulation defines 53 mm as the absolute minimum marketable orange size. Anything smaller is not a valid orange for sale.
- `deformation > 20%` — This corresponds to Class III, the last accepted category. Anything beyond this is too irregular to classify meaningfully (likely noise, partial blob, or a non-orange object).

### Example
```
Blob A: diameter=65mm, deformation=7%   → KEEP   (Class I shape)
Blob B: diameter=48mm, deformation=5%   → DISCARD (too small)
Blob C: diameter=70mm, deformation=28%  → DISCARD (too deformed)
Blob D: diameter=59mm, deformation=15%  → KEEP   (Class II shape)
```

---

## 10. Count Dark Spots  (`vc_blob_count_dark_spots`)

### What it does
Counts how many **spot pixels** (from the spot binary image) fall inside each orange blob.
Uses the label image to know which blob each pixel belongs to.

### Pseudo-code
```
// Build a lookup: label number → blob index
for each blob i:
    idx[blob[i].label] = i

// Count spots per blob
for every pixel (x, y):
    if spot_binary[x, y] == WHITE:           // it's a dark spot
        lbl = label_image[x, y]              // which orange?
        blobs[idx[lbl]].dark_spot_area++     // add to that orange's count

// Compute ratio
for each blob i:
    total = blob.area + blob.dark_spot_area
    blob.spot_ratio = dark_spot_area / total × 100  (%)
```

### Example
```
Orange blob area = 95,000 px
Dark spot pixels inside it = 720 px

total = 95,000 + 720 = 95,720
spot_ratio = 720 / 95,720 × 100 = 0.75%

→ 0.5% < 0.75% < 1.0%  →  spots push category to Class I
```

---

## 11. Count New Oranges  (`vc_count_new_oranges`)

### Goal
Track how many **unique** oranges have appeared in the video, without counting the same orange multiple times as it moves through the frame.

### How it works
Each frame, the function compares the current blobs' centroids against the centroids from the **previous frame**.

```
for each blob in current frame:
    is_new = true
    for each centroid in previous frame:
        distance = sqrt((xc_curr - xc_prev)² + (yc_curr - yc_prev)²)
        if distance < 80 pixels:
            is_new = false   // same orange, just moved a bit
            break

    if is_new:
        total_count++

// Save current centroids as "previous" for next frame
```

### The 80-pixel threshold
An orange moving at normal conveyor belt speed moves less than 80 pixels between two consecutive frames. So if a centroid is within 80 px of one in the previous frame, it is the same orange.

### Example
```
Frame 100 centroids: (200, 360), (500, 360), (800, 360)
Frame 101 centroids: (190, 360), (490, 360), (795, 360), (1050, 360)

Orange at (1050, 360): no previous centroid within 80px → NEW → total++
Others: matched to frame 100 → not new
```

---

## 12. Uniformity Check (in `main.cpp`)

Not a separate function, but worth understanding for the presentation.

### Global min/max tracking
Every frame, after filtering:
```
for each blob in filtered_blobs:
    if blob.diameter_mm > global_max → update global_max
    if blob.diameter_mm < global_min → update global_min
```
Since min and max naturally settle to the true extremes, measuring the same orange multiple times across frames does not distort the result.

### EEC 379/71 uniformity rule
The regulation limits how different the smallest and largest orange in a batch can be:

| Situation | Max allowed difference |
|-----------|------------------------|
| All oranges ≥ 73 mm (calibres 1–8) | 10 mm |
| Any orange < 73 mm (calibres 9–13) | 7 mm |

```
size_diff = global_max_diameter - global_min_diameter

if global_min >= 73mm:
    tolerance = 10mm
else:
    tolerance = 7mm

if size_diff <= tolerance → CONFORME
else                      → NAO CONFORME
```

### Example
```
Smallest orange seen: 61.2 mm  →  below 73mm  →  tolerance = 7mm
Largest orange seen:  70.8 mm

size_diff = 70.8 - 61.2 = 9.6 mm
9.6 > 7.0  →  NAO CONFORME
```

---

## Quick Reference — Constants in `vc.h`

| Constant | Value | Meaning |
|----------|-------|---------|
| `VC_PX_TO_MM` | 55/280 ≈ 0.196 | pixel to mm scale |
| `VC_ORANGE_H_MIN/MAX` | 15 – 35 | orange hue range (degrees) |
| `VC_ORANGE_S_MIN/MAX` | 33 – 100 | orange saturation range (%) |
| `VC_ORANGE_V_MIN/MAX` | 25 – 100 | orange brightness range (%) |
| `VC_SPOT_KERNEL_LOCAL` | 31 | neighbourhood window for spot detection |
| `VC_SPOT_THRESHOLD` | 8 | how much darker than neighbours = a spot |
| `VC_DEFORM_EXTRA/I/II` | 5 / 10 / 20% | deformation category limits |
| `VC_SPOT_EXTRA/I/II` | 0.5 / 1.0 / 2.0% | spot ratio category limits |
| `VC_MIN_DIAMETER_MM` | 53 mm | minimum valid orange size |
| `VC_FILTER_DEFORM_MAX` | 20% | maximum deformation before blob is discarded |
| `VC_DIST_MAX` | 80 px | max movement between frames (tracking) |
| `VC_UNIFORM_TOL_LARGE` | 10 mm | uniformity tolerance for large calibres |
| `VC_UNIFORM_TOL_SMALL` | 7 mm | uniformity tolerance for small calibres |
| `VC_UNIFORM_BOUNDARY_MM` | 73 mm | threshold between large/small calibre groups |
