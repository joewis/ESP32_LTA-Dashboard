// ESP32 Bus Dashboard Enclosure v2
// Stacked layout: display → e-paper PCB → ESP32 + battery
// Max outside depth: 40mm
//
// Hardware:
//   - WeAct 4.2" e-paper module (GDEY042Z98 tri-color)
//   - ESP32 dev board w/ CH340, 45×25mm + 10mm antenna
//   - 146074 LiPo battery: 14×60×74mm (10000mAh)

/* [Display Module - WeAct 4.2" e-paper] */
epd_pcb_w = 89.6;      // mm — PCB width
epd_pcb_h = 91.6;      // mm — PCB height
epd_pcb_t = 1.6;       // mm — PCB thickness
// Display active area
disp_w = 84.8;          // mm
disp_h = 63.6;          // mm
disp_off_x = (epd_pcb_w - disp_w) / 2;  // centered
disp_off_y = (epd_pcb_h - disp_h) / 2;  // centered
// Connector ribbon on e-paper PCB (bottom edge, protrudes back)
ribbon_w = 30.0;        // mm
ribbon_h = 12.0;        // mm — protrusion behind PCB
// Mounting holes
hole_d = 3.0;           // mm
hole_off_x = 6.4;       // mm from left/right edge
hole_off_y_bot = 2.8;   // mm from bottom edge
hole_off_y_top = 1.93;  // mm from top edge

/* [ESP32 Dev Board - CH340 variant] */
esp32_w = 55.0;         // mm (45 board + 10 antenna)
esp32_h = 25.0;         // mm
esp32_t = 12.0;         // mm (thickness including components + pins)
esp32_usb_w = 12.0;     // mm — USB-C port width
esp32_usb_h = 6.0;      // mm — USB-C port height
esp32_usb_protrude = 3.0; // mm — USB-C protrudes beyond board edge

/* [Battery - 146074 LiPo] */
bat_w = 74.0;           // mm (length)
bat_h = 60.0;           // mm (width)
bat_t = 14.0;           // mm (thickness)

/* [Case Parameters] */
wall = 2.0;             // mm — wall thickness
front_t = 2.0;          // mm — front plate (display window lip)
tolerance = 0.4;        // mm — clearance around components
max_depth = 40.0;       // mm — max outside depth constraint

/* [Back Cover - snap fit] */
cover_t = 1.5;         // mm — back cover thickness
cover_lip = 1.2;       // mm — snap-fit lip depth
cover_gap = 0.3;       // mm — clearance between body and cover

/* [Ventilation] */
vent_slots = 8;         // slots on back panel
vent_w = 2.5;           // mm — slot width
vent_h = 40.0;          // mm — slot height

/* [Wall Mount] */
mount_hole_d = 5.0;     // mm — wall screw hole diameter
mount_hole_off = 12.0;   // mm from corners

/* [Printer Settings] */
layer_h = 0.2;          // mm

// ─── Computed Stackup (front to back) ───
// front_t + epd_pcb_t + gap + esp32_t + gap + bat_t + base_t <= max_depth
gap = 0.5;              // mm — gap between layers for ribbon/connector clearance
stack_min = front_t + epd_pcb_t + gap + ribbon_h + gap + esp32_t + gap + bat_t + base_t;
echo(str("Minimum stack depth: ", stack_min, " mm"));
echo(str("Max allowed depth: ", max_depth, " mm"));

// If stack exceeds max, reduce gaps or base_t
// 2 + 1.6 + 0.5 + 12 + 0.5 + 12 + 0.5 + 14 + 2 = 45.1 — over by 5.1mm
// Battery and ESP32 must sit side-by-side, not stacked!

// ─── Revised Layout: Battery beside ESP32 ───
// Depth: front_t + epd_pcb_t + gap + max(esp32_t, bat_t) + gap + base_t
// But bat_t (14mm) > esp32_t (12mm), so:
// 2 + 1.6 + 0.5 + 14 + 0.5 + 2 = 20.6mm — plenty of room!
// ESP32 sits in same cavity as battery, side by side
// Battery is thicker so it sets the cavity depth

cavity_depth = max(esp32_t, bat_t) + gap;  // 14.5mm
// Body = front wall + PCB + gap + cavity + gap + back wall
body_depth = front_t + epd_pcb_t + gap + cavity_depth + gap + wall;
// Total = body + cover lip recess + cover plate
outer_d = body_depth + cover_lip + cover_t;
cover_recess_z = body_depth;  // where the cover slots in
echo(str("Body depth: ", body_depth, " mm"));
echo(str("Total outer depth: ", outer_d, " mm"));
echo(str("Depth budget remaining: ", max_depth - outer_d, " mm"));

// Overall width/height set by display PCB + walls
outer_w = epd_pcb_w + tolerance * 2 + wall * 2;
outer_h = epd_pcb_h + tolerance * 2 + wall * 2;

// Component Z positions (front to back)
epd_z = front_t;                           // e-paper PCB sits right behind front wall
cavity_z = epd_z + epd_pcb_t + gap;         // cavity starts after PCB + ribbon clearance
esp32_z = cavity_z + (bat_t - esp32_t) / 2; // ESP32 vertically centered with battery
bat_z = cavity_z;                           // battery at bottom of cavity
base_z = body_depth;                        // back wall (cover recess starts here)

// X positions — battery and ESP32 side by side in cavity
// Battery (74mm wide) + ESP32 (55mm wide) = 129mm total
// But cavity inner width is ~90mm (epd_pcb_w + tolerance) — they can't fit side by side!
// Battery is 60mm HIGH (Y dimension), 74mm LONG (X dimension)
// Reorient: battery lying flat, 74mm along X, 60mm along Y, 14mm thick (Z)
// Still 74mm > 89.6mm cavity width? No, 74 < 90, battery fits along X.
// ESP32: 55mm along X, 25mm along Y — fits in remaining Y space next to battery.

bat_x = wall + tolerance;                    // battery against left wall
bat_y = wall + tolerance;                    // battery at bottom

esp32_x = (outer_w - esp32_w - tolerance * 2) / 2;  // ESP32 centered horizontally
// ESP32 sits above battery in Y (battery is 60mm, ESP32 is 25mm)
// Actually: place ESP32 offset to one side, battery on the other
// Better: battery centered, ESP32 at top

// Re-think: The cavity is 90×92mm. Battery is 74×60mm. ESP32 is 55×25mm.
// Best layout: Battery centered on X, flush against back.
// ESP32 above battery (in Y), centered.

bat_x = wall + tolerance + (epd_pcb_w - bat_w) / 2;  // center battery horizontally
bat_y = wall + tolerance;                              // battery at bottom of cavity

esp32_x = wall + tolerance + (epd_pcb_w - esp32_w) / 2;  // center ESP32 horizontally
esp32_y = bat_y + bat_h + gap;                             // ESP32 above battery

// Check: bat_y(2.4) + bat_h(60) + gap(0.5) + esp32_h(25) = 87.9 < 92.4 (cavity inner height) ✓

// USB cutout — on the top edge (since ESP32 is at top of cavity)
// USB-C protrudes 3mm from the TOP edge of the ESP32 board
usb_cut_x = esp32_x + (esp32_w - esp32_usb_w) / 2;  // centered on ESP32
usb_cut_y = outer_h - wall;                           // top wall
usb_cut_z = esp32_z;                                  // at ESP32 depth

// ─── Modules ───

module display_window() {
    // Cut through front wall for e-ink display
    win_x = wall + tolerance + disp_off_x;
    win_y = wall + tolerance + disp_off_y;
    translate([win_x, win_y, -0.1])
        cube([disp_w, disp_h, front_t + 0.2]);
}

module display_recess() {
    // Slight recess so e-ink glass sits flush with front face
    win_x = wall + tolerance + disp_off_x - 1;
    win_y = wall + tolerance + disp_off_y - 1;
    translate([win_x, win_y, front_t - 0.5])
        cube([disp_w + 2, disp_h + 2, 0.6]);
}

module mounting_holes_front() {
    // Holes for screws through e-paper PCB into standoffs
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + tolerance + x, outer_h - wall - tolerance - y, -1])
                cylinder(d = hole_d, h = front_t + 2, $fn = 16);
        }
    }
}

module standoffs() {
    // Standoffs behind e-paper PCB for mounting screws
    standoff_h = gap + 2;  // enough to clear ribbon connector
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + tolerance + x, outer_h - wall - tolerance - y, epd_z + epd_pcb_t]) {
                difference() {
                    cylinder(d = hole_d + 5, h = standoff_h, $fn = 16);
                    translate([0, 0, -0.5])
                        cylinder(d = hole_d, h = standoff_h + 1, $fn = 16);
                }
            }
        }
    }
}

module usb_cutout() {
    // USB-C port opening on top edge
    translate([usb_cut_x, usb_cut_y - 0.5, usb_cut_z])
        cube([esp32_usb_w, wall + 1.5, esp32_usb_h]);
}

module battery_cavity() {
    translate([bat_x, bat_y, bat_z])
        cube([bat_w + tolerance, bat_h + tolerance, bat_t + gap]);
}

module esp32_cavity() {
    translate([esp32_x, esp32_y, esp32_z])
        cube([esp32_w + tolerance, esp32_h + tolerance, esp32_t + gap]);
}

module ventilation_slots() {
    // Slots on back panel for air circulation
    slot_spacing = (epd_pcb_w - 20) / (vent_slots + 1);
    for (i = [1:vent_slots]) {
        sx = wall + 10 + i * slot_spacing;
        translate([sx, wall + 15, base_z - 0.1])
            cube([vent_w, vent_h, base_t + 0.2]);
    }
}

module wall_mount_holes() {
    // Four wall-mount screw holes through back plate
    for (x = [mount_hole_off, outer_w - mount_hole_off]) {
        for (y = [mount_hole_off, outer_h - mount_hole_off]) {
            translate([x, y, base_z - 0.1])
                cylinder(d = mount_hole_d, h = base_t + 0.3, $fn = 16);
            // Countersink
            translate([x, y, base_z - 0.5])
                cylinder(d1 = mount_hole_d, d2 = mount_hole_d + 3, h = 1.0, $fn = 16);
        }
    }
}

module internal_ribs() {
    // Support ribs between e-paper PCB and back panel
    rib_w = 2.0;
    rib_positions = [
        [wall + 5, wall + tolerance + 5],
        [outer_w - wall - 5 - rib_w, wall + tolerance + 5],
        [wall + 5, outer_h - wall - tolerance - 5 - 20],
        [outer_w - wall - 5 - rib_w, outer_h - wall - tolerance - 5 - 20]
    ];
    for (p = rib_positions) {
        translate([p[0], p[1], cavity_z])
            cube([rib_w, 20, cavity_depth]);
    }
}

// ─── Back Cover (print separately, snap-fit) ───
module back_cover() {
    // Cover plate that snaps into the recess at the back
    // Slightly smaller than the body's back opening for clearance
    cover_w = outer_w - wall * 2 + cover_lip * 2 - cover_gap * 2;
    cover_h = outer_h - wall * 2 + cover_lip * 2 - cover_gap * 2;
    
    // Main plate
    translate([wall - cover_lip + cover_gap, wall - cover_lip + cover_gap, cover_recess_z])
        cube([cover_w, cover_h, cover_t]);
    
    // Snap-fit lips on all four edges
    lip_ext = cover_lip + cover_gap;
    // Top and bottom lips
    for (y_off = [wall - lip_ext, outer_h - wall + cover_gap]) {
        translate([wall - cover_lip + cover_gap, y_off, cover_recess_z])
            cube([cover_w, cover_lip, cover_t + 0.5]);
    }
    // Left and right lips
    for (x_off in [wall - lip_ext, outer_w - wall + cover_gap]) {
        translate([x_off, wall - cover_lip + cover_gap, cover_recess_z])
            cube([cover_lip, cover_h, cover_t + 0.5]);
    }
    
    // Ventilation slots through cover
    slot_spacing = (epd_pcb_w - 20) / (vent_slots + 1);
    for (i = [1:vent_slots]) {
        sx = wall + 10 + i * slot_spacing;
        translate([sx, wall + 15, cover_recess_z - 0.1])
            cube([vent_w, vent_h, cover_t + 0.3]);
    }
    
    // Wall-mount holes through cover
    for (x = [mount_hole_off, outer_w - mount_hole_off]) {
        for (y = [mount_hole_off, outer_h - mount_hole_off]) {
            translate([x, y, cover_recess_z - 0.1])
                cylinder(d = mount_hole_d, h = cover_t + 0.3, $fn = 16);
        }
    }
}

// ─── Main Body ───
difference() {
    // Outer shell
    cube([outer_w, outer_h, body_depth]);
    
    // Main cavity (behind e-paper PCB)
    translate([wall, wall, cavity_z])
        cube([outer_w - wall * 2, outer_h - wall * 2, cavity_depth + gap]);
    
    // Back cover recess (pocket at the back wall)
    translate([wall - cover_lip, wall - cover_lip, cover_recess_z])
        cube([outer_w - wall * 2 + cover_lip * 2, outer_h - wall * 2 + cover_lip * 2, cover_lip + cover_t]);
    
    // Display window
    display_window();
    
    // Display recess (slight step for glass)
    display_recess();
    
    // USB-C cutout on top edge
    usb_cutout();
    
    // Battery pocket
    battery_cavity();
    
    // ESP32 pocket
    esp32_cavity();
    
    // Front mounting holes (for PCB screws)
    mounting_holes_front();
}

// Standoffs for e-paper PCB
standoffs();

// Internal support ribs
internal_ribs();

// Back cover (comment out for body-only print)
// back_cover();
// Uncomment to render cover separately:
// translate([0, outer_h + 10, 0]) back_cover();

// ─── Print Info ───
echo("========================================");
echo(str("  BODY: ", outer_w, " x ", outer_h, " x ", body_depth, " mm"));
echo(str("  TOTAL: ", outer_w, " x ", outer_h, " x ", outer_d, " mm (with cover)"));
echo(str("  Cavity depth: ", cavity_depth, " mm"));
echo(str("  Depth budget: ", outer_d, "/", max_depth, " mm (", max_depth - outer_d, " mm spare)"));
echo(str("  Battery: ", bat_w, "x", bat_h, "x", bat_t, " at (", bat_x, ", ", bat_y, ")"));
echo(str("  ESP32:  ", esp32_w, "x", esp32_h, "x", esp32_t, " at (", esp32_x, ", ", esp32_y, ")"));
echo(str("  USB-C cutout on top edge, centered on ESP32"));
echo(str("  Print: body face-DOWN, cover face-DOWN"));
echo(str("  ~", ceil(body_depth / layer_h), "/", ceil(outer_d / layer_h), " layers at ", layer_h, "mm"));
echo("========================================");