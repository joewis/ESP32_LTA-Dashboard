// ESP32 Bus Dashboard Enclosure
// Parametric OpenSCAD model
// Customize dimensions below for your specific boards

/* [Dimensions - E-Paper Module] */
// WeAct 4.2" e-paper module PCB size
epd_pcb_w = 89.6;     // mm — PCB width
epd_pcb_h = 91.6;     // mm — PCB height
epd_pcb_t = 1.6;      // mm — PCB thickness

// Display window (visible area of e-ink)
disp_w = 84.8;         // mm
disp_h = 63.6;         // mm
// Offset from top-left of PCB to display area
disp_off_x = (epd_pcb_w - disp_w) / 2;
disp_off_y = (epd_pcb_h - disp_h) / 2;

// Mounting hole diameter
hole_d = 3.0;          // mm
// Hole positions from edges
hole_off_x = 6.4;      // mm from left/right
hole_off_y_bot = 2.8;  // mm from bottom
hole_off_y_top = 1.93; // mm from top (from PDF)

/* [Dimensions - ESP32 Dev Board] */
// CH340 ESP32 dev board (generic)
esp32_w = 51.0;        // mm
esp32_h = 26.0;        // mm
esp32_t = 12.0;        // mm (thickness including components)
esp32_usb_off = 5.0;   // mm — USB port setback from board edge
esp32_usb_w = 12.0;    // mm — USB port width
esp32_usb_h = 8.0;     // mm — USB port protrusion

/* [Case Parameters] */
wall = 2.0;            // mm — wall thickness
base_t = 1.6;          // mm — base plate thickness
tolerance = 0.3;       // mm — clearance around PCBs

/* [Ventilation] */
vent_slots = 6;        // number of ventilation slots
vent_w = 3.0;          // mm — slot width

/* [Printer Settings] */
layer_h = 0.2;         // mm — print layer height (for reference)

// ─── Computed values ───
inner_w = epd_pcb_w + tolerance * 2;
inner_h = epd_pcb_h + tolerance * 2;
inner_d = esp32_t + wall * 2 + tolerance * 2;

outer_w = inner_w + wall * 2;
outer_h = inner_h + wall * 2;
outer_d = inner_d + wall;

// USB cutout position (bottom edge, centered)
usb_cut_x = (outer_w - esp32_usb_w) / 2;
usb_cut_y = outer_h - wall - tolerance;
usb_cut_z = wall + base_t;

// ─── Modules ───
module display_window() {
    // Recess for the e-ink display glass area
    win_x = wall + disp_off_x;
    win_y = outer_h - wall - disp_off_y - disp_h;
    translate([win_x, win_y, outer_d - wall])
        cube([disp_w, disp_h, wall + 0.5]);
}

module mounting_holes() {
    // Four corner holes
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + x, outer_h - wall - y, -1])
                cylinder(d = hole_d, h = outer_d + 2, $fn = 16);
        }
    }
}

module usb_cutout() {
    // USB port opening on the bottom edge
    translate([usb_cut_x, usb_cut_y - 1, usb_cut_z])
        cube([esp32_usb_w, wall + 2, esp32_usb_h + 1]);
}

module esp32_cavity() {
    // Pocket where ESP32 sits
    // Centered below the e-paper module
    cavity_x = (outer_w - esp32_w - tolerance * 2) / 2;
    cavity_y = (outer_h - esp32_h - tolerance * 2) / 2;
    translate([cavity_x, cavity_y, base_t])
        cube([esp32_w + tolerance * 2, esp32_h + tolerance * 2, esp32_t + tolerance]);
}

module ventilation_slots() {
    // Slots on the back (top face)
    slot_spacing = (inner_w - 10) / (vent_slots + 1);
    for (i = [1:vent_slots]) {
        sx = wall + 5 + i * slot_spacing;
        translate([sx, wall + 10, outer_d - wall - 0.1])
            cube([vent_w, inner_h - 20, wall + 0.2]);
    }
}

module standoffs() {
    // Screw standoffs in the four corners
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([wall + x, outer_h - wall - y, base_t]) {
                difference() {
                    cylinder(d = hole_d + 4, h = inner_d - base_t, $fn = 16);
                    cylinder(d = hole_d, h = inner_d - base_t + 1, $fn = 16);
                }
            }
        }
    }
}

// ─── Main Body ───
difference() {
    // Outer shell
    cube([outer_w, outer_h, outer_d]);
    
    // Inner cavity
    translate([wall, wall, wall])
        cube([inner_w, inner_h, inner_d]);
    
    // Display window recess
    display_window();
    
    // USB cutout
    usb_cutout();
    
    // Ventilation slots
    ventilation_slots();
    
    // Mounting holes
    mounting_holes();
    
    // ESP32 cavity
    esp32_cavity();
}

// Standoffs (added back)
standoffs();

// ─── Print Info ───
echo(str("Outer dimensions: ", outer_w, " x ", outer_h, " x ", outer_d, " mm"));
echo(str("Print with the display window facing UP (no supports needed)"));
echo(str("Layer height: ", layer_h, " mm — ~", ceil(outer_d / layer_h), " layers"));
echo(str("USB cutout at bottom edge, centered"));
