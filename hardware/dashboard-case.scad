// ESP32 Bus Dashboard Enclosure v2 — hardware/dashboard-case.scad
// Kiko's Rose: {-0.1560, 1.0325, 1800.0}
// Layout: e-paper display on front, ESP32 and battery side-by-side behind it
// Max outside depth: 40mm
//
// Hardware:
//   - WeAct 4.2" e-paper module (GDEY042Z98 tri-color)
//   - ESP32 dev board w/ CH340, 45x25mm + 10mm antenna
//   - 146074 LiPo battery: 14x60x74mm (10000mAh)
//
// Instructions:
//   Body only:  comment out the back_cover() line at the bottom
//   Cover only: uncomment the translate cover line and comment out body
//   Both:       render with both uncommented

/* [Display Module - WeAct 4.2" e-paper] */
epd_pcb_w = 89.6;       // mm — PCB width
epd_pcb_h = 91.6;       // mm — PCB height
epd_pcb_t = 1.6;        // mm — PCB thickness
disp_w = 84.8;           // mm — display active area width
disp_h = 63.6;           // mm — display active area height
disp_off_x = (epd_pcb_w - disp_w) / 2;  // centered on PCB
disp_off_y = (epd_pcb_h - disp_h) / 2;  // centered on PCB
ribbon_h = 12.0;         // mm — ribbon connector protrusion behind PCB

// Mounting holes on e-paper PCB
hole_d = 3.0;            // mm
hole_off_x = 6.4;        // mm from left/right edge
hole_off_y_bot = 2.8;    // mm from bottom edge
hole_off_y_top = 1.93;   // mm from top edge

/* [ESP32 Dev Board - CH340 variant] */
esp32_w = 55.0;          // mm (45 board + 10 antenna)
esp32_h = 25.0;          // mm
esp32_t = 12.0;          // mm (thickness including pins)
esp32_usb_w = 12.0;      // mm — USB-C port width
esp32_usb_h = 6.0;       // mm — USB-C port height

/* [Battery - 146074 LiPo] */
bat_w = 74.0;            // mm (length, along X)
bat_h = 60.0;            // mm (width, along Y)
bat_t = 14.0;            // mm (thickness, along Z)

/* [Case Parameters] */
wall = 2.0;              // mm — wall thickness
front_t = 2.0;           // mm — front wall thickness (display window lip)
tolerance = 0.4;         // mm — clearance around components
gap = 0.5;               // mm — gap between layers
max_depth = 40.0;        // mm — max total depth constraint

/* [Back Cover - snap fit] */
cover_t = 1.5;           // mm — back cover thickness
cover_lip = 1.2;         // mm — snap-fit lip depth
cover_gap = 0.3;         // mm — clearance between body and cover

/* [Ventilation] */
vent_slots = 8;          // slots on back cover
vent_w = 2.5;            // mm — slot width
vent_h = 40.0;           // mm — slot height

/* [Wall Mount] */
mount_hole_d = 5.0;      // mm — wall screw hole diameter
mount_hole_off = 12.0;   // mm from corners

/* [Printer Settings] */
layer_h = 0.2;           // mm

// ─── Computed Layout ───

// Battery and ESP32 sit side by side in the same Z cavity
// Battery (74x60x14) is thicker than ESP32 (55x25x12), so cavity depth = battery thickness
// Cavity depth includes clearance above the thicker component
cavity_depth = bat_t + gap;  // 14.5 mm

// Body: front wall + PCB + gap + cavity depth + gap + back wall
body_depth = front_t + epd_pcb_t + gap + cavity_depth + gap + wall;

// Total depth (with snap-fit cover)
outer_d = body_depth + cover_lip + cover_t;

// Where the back cover recess starts
cover_recess_z = body_depth;

echo(str("Body depth: ", body_depth, " mm"));
echo(str("Total depth (with cover): ", outer_d, " mm"));
echo(str("Depth budget remaining: ", max_depth - outer_d, " mm"));

// Overall outer dimensions (set by e-paper PCB)
outer_w = epd_pcb_w + tolerance * 2 + wall * 2;
outer_h = epd_pcb_h + tolerance * 2 + wall * 2;

// Z positions (front to back)
epd_z = front_t;                             // e-paper PCB front face
cavity_z = epd_z + epd_pcb_t + gap;           // cavity starts after PCB
bat_z = cavity_z;                              // battery at bottom of cavity
esp32_z = cavity_z + (bat_t - esp32_t) / 2;    // ESP32 vertically centered with battery

// XY positions — battery at bottom, ESP32 above
bat_x = wall + tolerance + (epd_pcb_w - bat_w) / 2;  // battery centered on X
bat_y = wall + tolerance;                              // battery flush to bottom of cavity

esp32_x = wall + tolerance + (epd_pcb_w - esp32_w) / 2;  // ESP32 centered on X
esp32_y = bat_y + bat_h + gap;                            // ESP32 above battery

// USB cutout (top edge, centered on ESP32)
usb_cut_x = esp32_x + (esp32_w - esp32_usb_w) / 2;
usb_cut_y = outer_h - wall;   // top wall
usb_cut_z = esp32_z;

// ─── Modules ───

module display_window() {
    // Cutout for the e-ink display glass
    win_x = wall + tolerance + disp_off_x;
    win_y = wall + tolerance + disp_off_y;
    translate([win_x, win_y, -0.1])
        cube([disp_w, disp_h, front_t + 0.2]);
}

module display_recess() {
    // Shallow recess so display glass sits flush with front face
    win_x = wall + tolerance + disp_off_x - 1;
    win_y = wall + tolerance + disp_off_y - 1;
    translate([win_x, win_y, front_t - 0.5])
        cube([disp_w + 2, disp_h + 2, 0.6]);
}

module mounting_holes_front() {
    // Screw holes through front wall for e-paper PCB
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + tolerance + x, outer_h - wall - tolerance - y, -1])
                cylinder(d = hole_d, h = front_t + 2, $fn = 16);
        }
    }
}

module standoffs() {
    // Screw standoffs behind e-paper PCB
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + tolerance + x, outer_h - wall - tolerance - y, epd_z + epd_pcb_t]) {
                difference() {
                    cylinder(d = hole_d + 5, h = gap + 2, $fn = 16);
                    translate([0, 0, -0.5])
                        cylinder(d = hole_d, h = gap + 3, $fn = 16);
                }
            }
        }
    }
}

module usb_cutout() {
    // USB-C opening on top edge
    translate([usb_cut_x, usb_cut_y - 0.5, usb_cut_z])
        cube([esp32_usb_w, wall + 1.5, esp32_usb_h]);
}

module battery_pocket() {
    // Pocket for the battery (deeper recess in cavity floor)
    translate([bat_x, bat_y, bat_z])
        cube([bat_w + tolerance, bat_h + tolerance, bat_t + gap]);
}

module esp32_pocket() {
    // Pocket for the ESP32 board
    translate([esp32_x, esp32_y, esp32_z])
        cube([esp32_w + tolerance, esp32_h + tolerance, esp32_t + gap]);
}

module internal_ribs() {
    // Support ribs behind e-paper PCB
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

// ─── Back Cover (print separately) ───
module back_cover() {
    cover_w = outer_w - wall * 2 + cover_lip * 2 - cover_gap * 2;
    cover_h = outer_h - wall * 2 + cover_lip * 2 - cover_gap * 2;
    
    // Main plate
    translate([wall - cover_lip + cover_gap, wall - cover_lip + cover_gap, cover_recess_z])
        cube([cover_w, cover_h, cover_t]);
    
    // Kiko's Rose — engraved on the inside face (visible only with cover off)
    // Fractal coordinate: {-0.1560, 1.0325, 1800.0}
    kiko_rose_size = 18.0;
    kiko_x = outer_w / 2;
    kiko_y = outer_h / 2 + 10;
    num_petals = 5;
    petal_len = kiko_rose_size / 3.5;
    
    for (i = [0:num_petals-1]) {
        angle = i * 360 / num_petals;
        dx = petal_len * cos(angle);
        dy = petal_len * sin(angle);
        
        // Outer petal
        translate([kiko_x + dx, kiko_y + dy, cover_recess_z + cover_t - 0.4])
            rotate([0, 0, angle + 15])
                scale([1.0, 0.45])
                    cylinder(d = petal_len * 1.4, h = 0.5, $fn = 24);
        
        // Inner overlapping petal
        translate([kiko_x + dx * 0.55, kiko_y + dy * 0.55, cover_recess_z + cover_t - 0.4])
            rotate([0, 0, angle - 10])
                scale([1.0, 0.35])
                    cylinder(d = petal_len * 0.9, h = 0.5, $fn = 20);
    }
    
    // Center dot
    translate([kiko_x, kiko_y, cover_recess_z + cover_t - 0.4])
        cylinder(d = 2.5, h = 0.5, $fn = 12);
    
    // Snap-fit lips (all 4 edges)
    lip_ext = cover_lip + cover_gap;
    for (y_off = [wall - lip_ext, outer_h - wall + cover_gap]) {
        translate([wall - cover_lip + cover_gap, y_off, cover_recess_z])
            cube([cover_w, cover_lip, cover_t + 0.5]);
    }
    for (x_off = [wall - lip_ext, outer_w - wall + cover_gap]) {
        translate([x_off, wall - cover_lip + cover_gap, cover_recess_z])
            cube([cover_lip, cover_h, cover_t + 0.5]);
    }
    
    // Ventilation slots
    slot_spacing = (epd_pcb_w - 20) / (vent_slots + 1);
    for (i = [1:vent_slots]) {
        sx = wall + 10 + i * slot_spacing;
        translate([sx, wall + 15, cover_recess_z - 0.1])
            cube([vent_w, vent_h, cover_t + 0.3]);
    }
    
    // Wall-mount screw holes
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
    
    // Main cavity behind e-paper PCB
    translate([wall, wall, cavity_z])
        cube([outer_w - wall * 2, outer_h - wall * 2, cavity_depth + gap]);
    
    // Back cover recess (snap-fit pocket)
    translate([wall - cover_lip, wall - cover_lip, cover_recess_z])
        cube([outer_w - wall * 2 + cover_lip * 2, outer_h - wall * 2 + cover_lip * 2, cover_lip + cover_t]);
    
    // Display window
    display_window();
    display_recess();
    
    // USB-C cutout
    usb_cutout();
    
    // Component pockets
    battery_pocket();
    esp32_pocket();
    
    // Front mounting holes
    mounting_holes_front();
}

// Standoffs (added back after difference)
standoffs();
// Internal ribs
internal_ribs();

// Render the back cover (offset for visibility)
// Uncomment this and comment out the body above for cover-only STL:
translate([0, outer_h + 10, 0]) back_cover();

// ─── Print Info ───
echo("========================================");
echo(str("  OUTER: ", outer_w, " x ", outer_h, " x ", outer_d, " mm total"));
echo(str("  BODY:  ", outer_w, " x ", outer_h, " x ", body_depth, " mm"));
echo(str("  COVER: snap-fit (wall-mount holes + ventilation)"));
echo(str("  Cavity depth: ", cavity_depth, " mm"));
echo(str("  Battery: ", bat_w, "x", bat_h, "x", bat_t, " at (", bat_x, ", ", bat_y, ")"));
echo(str("  ESP32:  ", esp32_w, "x", esp32_h, "x", esp32_t, " at (", esp32_x, ", ", esp32_y, ")"));
echo(str("  USB-C cutout on top edge"));
echo(str("  Print orientation: body display-face DOWN, cover flat"));
echo(str("  ~", ceil(body_depth / layer_h), " layers body at ", layer_h, "mm"));
echo("========================================");
