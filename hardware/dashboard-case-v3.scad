// ESP32 Bus Dashboard Enclosure v3
// Kiko's Rose: {-0.1560, 1.0325, 1800.0}
//
// Two-part snap-fit design:
//   1) Bezel  — frame that screws onto e-paper PCB via mounting holes
//   2) Box   — 40mm deep enclosure with recess for display, pin holes for bezel
//
// Hardware:
//   - WeAct 4.2" e-paper module (GDEY042Z98 tri-color), PCB 89.6x91.6x1.6mm
//   - ESP32 dev board w/ CH340, 45x25mm + 10mm antenna
//   - 146074 LiPo battery: 14x60x74mm (10000mAh)

/* [Display Module - WeAct 4.2" e-paper] */
epd_pcb_w = 89.6;       // mm — PCB width
epd_pcb_h = 91.6;       // mm — PCB height
epd_pcb_t = 1.6;        // mm — PCB thickness
disp_w = 84.8;           // mm — display active area width
disp_h = 63.6;           // mm — display active area height
disp_off_x = (epd_pcb_w - disp_w) / 2;  // centered
disp_off_y = (epd_pcb_h - disp_h) / 2;  // centered

// Mounting holes on e-paper PCB (screw hole positions)
hole_d = 3.0;            // mm
hole_off_x = 6.4;        // mm from left/right edge
hole_off_y_bot = 2.8;    // mm from bottom edge
hole_off_y_top = 1.93;   // mm from top edge

/* [ESP32 Dev Board] */
esp32_w = 55.0;          // mm (45 board + 10 antenna)
esp32_h = 25.0;          // mm
esp32_t = 12.0;          // mm (thickness with pins)
esp32_usb_w = 12.0;      // mm — USB-C port width
esp32_usb_h = 6.0;       // mm — USB-C port height

/* [Battery - 146074 LiPo] */
bat_w = 74.0;            // mm
bat_h = 60.0;            // mm
bat_t = 14.0;            // mm

/* [Case Parameters] */
wall = 2.0;              // mm — wall thickness
tolerance = 0.3;         // mm — clearance
gap = 0.5;               // mm — gap between layers
max_depth = 40.0;        // mm — max box depth

/* [Bezel / Box mating] */
pin_d = 3.0;             // mm — pin diameter (matching PCB hole)
pin_len = 5.0;           // mm — pin length (for bezel)
recess_d = 2.0;          // mm — display recess depth in box top

/* [Ventilation] */
vent_slots = 8;          // slots on box back
vent_w = 2.5;            // mm — slot width
vent_h = 40.0;           // mm — slot height

/* [Wall Mount] */
mount_hole_d = 5.0;
mount_hole_off = 12.0;

/* [Printer Settings] */
layer_h = 0.2;

// ─── Computed Layout ───
// Battery and ESP32 side-by-side in the same Z plane
cavity_depth = bat_t + gap;  // 14.5mm

// Box outer dimensions (set by e-paper PCB + wall)
outer_w = epd_pcb_w + tolerance * 2 + wall * 2;
outer_h = epd_pcb_h + tolerance * 2 + wall * 2;
box_depth = max_depth;  // use the full 40mm depth

echo(str("Box outer: ", outer_w, " x ", outer_h, " x ", box_depth, " mm"));
echo(str("Depth remaining: ", max_depth - box_depth, " mm"));

// Z positions
cavity_z = wall;                             // cavity starts after front wall of box
bat_z = cavity_z;                              // battery at bottom of cavity
esp32_z = cavity_z + (bat_t - esp32_t) / 2;    // ESP32 vertically centered

// XY — battery at bottom, ESP32 above
bat_x = wall + tolerance + (epd_pcb_w - bat_w) / 2;
bat_y = wall + tolerance;
esp32_x = wall + tolerance + (epd_pcb_w - esp32_w) / 2;
esp32_y = bat_y + bat_h + gap;

// USB cutout (top face of box, centered on ESP32)
usb_cut_x = esp32_x + (esp32_w - esp32_usb_w) / 2;
usb_cut_y = outer_h - wall;  // top wall
usb_cut_z = esp32_z;

// Display recess on top of box (on the front face)
disp_recess_x = wall + tolerance + disp_off_x;
disp_recess_y = wall + tolerance + disp_off_y;

// Bezel dimensions (slightly larger than PCB)
bezel_w = epd_pcb_w + tolerance * 2;
bezel_h = epd_pcb_h + tolerance * 2;
bezel_t = 2.0;            // bezel thickness
bezel_frame_w = 3.0;      // frame width around display window

echo(str("Bezel: ", bezel_w, " x ", bezel_h, " x ", bezel_t, " mm"));

// ─── Modules ───

module bezel() {
    difference() {
        // Outer frame
        cube([bezel_w, bezel_h, bezel_t]);
        
        // Display window cutout
        translate([bezel_frame_w + disp_off_x, bezel_frame_w + disp_off_y, -0.1])
            cube([disp_w, disp_h, bezel_t + 0.2]);
        
        // Screw holes (pass through for PCB)
        for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
            for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
                translate([x, bezel_h - y, -0.1])
                    cylinder(d = hole_d, h = bezel_t + 0.3, $fn = 16);
            }
        }
        
        // Countersink for screw heads
        for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
            for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
                translate([x, bezel_h - y, bezel_t - 0.4])
                    cylinder(d1 = hole_d + 3, d2 = hole_d, h = 0.6, $fn = 16);
            }
        }
    }
    
    // Alignment pins (on bezel bottom, mate into box top)
    pin_h = 4.0;  // pin height (protrudes below bezel)
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([x, bezel_h - y, bezel_t - 0.1])
                cylinder(d = pin_d - 0.2, h = pin_h + 0.2, $fn = 16);
        }
    }
}

module display_recess_box() {
    // Recess on the TOP face of the box for the e-paper display to sit in
    translate([disp_recess_x, disp_recess_y, -0.1])
        cube([epd_pcb_w - disp_off_x * 2, epd_pcb_h - disp_off_y * 2, recess_d + 0.2]);
}

module pin_holes_box() {
    // Mating holes on box top face for bezel pins
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([x, outer_h - wall - tolerance - y, -0.1])
                cylinder(d = pin_d, h = pin_len + 0.3, $fn = 16);
        }
    }
}

module usb_cutout_box() {
    // USB-C opening on top edge of box
    translate([usb_cut_x, usb_cut_y - 0.5, usb_cut_z])
        cube([esp32_usb_w, wall + 1.5, esp32_usb_h]);
}

module battery_pocket_box() {
    translate([bat_x, bat_y, bat_z])
        cube([bat_w + tolerance, bat_h + tolerance, bat_t + gap]);
}

module esp32_pocket_box() {
    translate([esp32_x, esp32_y, esp32_z])
        cube([esp32_w + tolerance, esp32_h + tolerance, esp32_t + gap]);
}

module ventilation_slots_box() {
    // Slots on back face (bottom z) of box
    slot_spacing = (epd_pcb_w - 20) / (vent_slots + 1);
    for (i = [1:vent_slots]) {
        sx = wall + 10 + i * slot_spacing;
        translate([sx, wall + 15, box_depth - wall - 0.1])
            cube([vent_w, vent_h, wall + 0.3]);
    }
}

module wall_mount_holes_box() {
    for (x = [mount_hole_off, outer_w - mount_hole_off]) {
        for (y = [mount_hole_off, outer_h - mount_hole_off]) {
            translate([x, y, box_depth - wall - 0.1])
                cylinder(d = mount_hole_d, h = wall + 0.3, $fn = 16);
            translate([x, y, box_depth - wall - 0.5])
                cylinder(d1 = mount_hole_d, d2 = mount_hole_d + 3, h = 1.0, $fn = 16);
        }
    }
}

module kiko_rose_back() {
    // Engraved rose on inside of box back wall
    kiko_rose_size = 18.0;
    kiko_x = outer_w / 2;
    kiko_y = outer_h / 2 + 10;
    num_petals = 5;
    petal_len = kiko_rose_size / 3.5;
    
    for (i = [0:num_petals-1]) {
        angle = i * 360 / num_petals;
        dx = petal_len * cos(angle);
        dy = petal_len * sin(angle);
        translate([kiko_x + dx, kiko_y + dy, box_depth - 0.4])
            rotate([0, 0, angle + 15])
                scale([1.0, 0.45])
                    cylinder(d = petal_len * 1.4, h = 0.5, $fn = 24);
        translate([kiko_x + dx * 0.55, kiko_y + dy * 0.55, box_depth - 0.4])
            rotate([0, 0, angle - 10])
                scale([1.0, 0.35])
                    cylinder(d = petal_len * 0.9, h = 0.5, $fn = 20);
    }
    translate([kiko_x, kiko_y, box_depth - 0.4])
        cylinder(d = 2.5, h = 0.5, $fn = 12);
}

// ─── Bezel (print separately) ───
module render_bezel() {
    translate([0, 0, 0]) bezel();
}

// ─── Box (enclosure body) ───
module render_box() {
    difference() {
        // Outer shell
        cube([outer_w, outer_h, box_depth]);
        
        // Main cavity
        translate([wall, wall, cavity_z])
            cube([outer_w - wall * 2, outer_h - wall * 2, cavity_depth + gap]);
        
        // Display recess on top face
        display_recess_box();
        
        // Pinholes for bezel pins
        pin_holes_box();
        
        // USB cutout
        usb_cutout_box();
        
        // Battery pocket
        battery_pocket_box();
        
        // ESP32 pocket
        esp32_pocket_box();
        
        // Ventilation slots (back face)
        ventilation_slots_box();
        
        // Wall-mount holes (back face)
        wall_mount_holes_box();
    }
    
    // Kiko's Rose engraving on inside of back wall
    kiko_rose_back();
}

// ─── Render ───

// Bezel (comment out for box-only)
// translate([0, outer_h + 10, 0]) render_bezel();

// Box
render_box();

echo("========================================");
echo(str("  BOX:  ", outer_w, " x ", outer_h, " x ", box_depth, " mm"));
echo(str("  BEZEL: frame on e-paper PCB, pins mate into box"));
echo(str("  Cavity depth: ", cavity_depth, " mm"));
echo(str("  Display recess on box top face"));
echo(str("  USB-C on top edge"));
echo(str("  Print: box back-face DOWN, bezel flat DOWN"));
echo(str("  ~", ceil(box_depth / layer_h), " / ", ceil(bezel_t / layer_h), " layers"));
echo("========================================");
