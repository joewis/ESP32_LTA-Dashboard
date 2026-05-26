// ESP32 Bus Dashboard Enclosure v3
// Kiko's Rose: {-0.1560, 1.0325, 1800.0}
//
// Two-part design: bezel + box
//   Bezel: frame that sits on TOP of e-paper PCB, screws into it
//   Box: 40mm deep, walls fully wrap around PCB edges (nothing visible from side)
//   Assembly: display glass -> bezel -> PCB -> pins drop into box recess
//
// Hardware:
//   - WeAct 4.2" e-paper module (GDEY042Z98 tri-color), PCB 89.6x91.6x1.6mm
//   - ESP32 dev board w/ CH340, 45x25mm + 10mm antenna
//   - 146074 LiPo battery: 14x60x74mm (10000mAh)

/* [Display Module - WeAct 4.2" e-paper] */
epd_pcb_w = 89.6;       epd_pcb_h = 91.6;       epd_pcb_t = 1.6;
disp_w = 84.8;           disp_h = 63.6;
disp_off_x = (epd_pcb_w - disp_w) / 2;  disp_off_y = (epd_pcb_h - disp_h) / 2;
hole_d = 3.0;            // PCB mounting hole diameter
hole_off_x = 6.4;        hole_off_y_bot = 2.8;   hole_off_y_top = 1.93;

/* [ESP32 Dev Board] */
esp32_w = 55.0;  esp32_h = 25.0;  esp32_t = 12.0;
esp32_usb_w = 12.0;  esp32_usb_h = 6.0;

/* [Battery - 146074 LiPo] */
bat_w = 74.0;  bat_h = 60.0;  bat_t = 14.0;

/* [Bezel] */
bezel_t = 2.0;           // thickness
bezel_w = epd_pcb_w + 0.6;  // 90.2mm — slightly larger than PCB
bezel_h = epd_pcb_h + 0.6;  // 92.2mm
b_win_w = disp_w;        // display window in bezel = display active area
b_win_h = disp_h;
b_win_x = (bezel_w - b_win_w) / 2;
b_win_y = (bezel_h - b_win_h) / 2;
pin_d = 3.0;            // alignment pin diameter
pin_h = 6.0;            // pin height (through PCB into box)

/* [Box] */
wall = 2.0;              // wall thickness
tolerance = 0.3;         // clearance
gap = 0.5;               // between layers
max_depth = 40.0;        // total depth

// Box inner cavity is LARGER than PCB/bezel — walls wrap around them
// bezel_inset: how far from the outer wall the bezel sits
bezel_inset = 5.0;       // mm — walls extend past bezel on all 4 sides
cav_inner_w = bezel_w + bezel_inset * 2;  // 100.2mm
cav_inner_h = bezel_h + bezel_inset * 2;  // 102.2mm
outer_w = cav_inner_w + wall * 2;           // 104.2mm
outer_h = cav_inner_h + wall * 2;           // 106.2mm
box_depth = max_depth;

// Top recess: pocket where the bezel + PCB sandwich drops in, flush with top rim
recess_depth = bezel_t + epd_pcb_t + gap;   // 4.1mm
recess_w = bezel_w + 1.0;                   // 91.2mm — a little clearance
recess_h = bezel_h + 1.0;                   // 93.2mm
recess_x = (outer_w - recess_w) / 2;
recess_y = (outer_h - recess_h) / 2;
// Display glass sits slightly below the bezel face
glass_recess = 0.3;

// Cavity for ESP32 and battery starts below the recess
cavity_z = wall + recess_depth;
cavity_depth = bat_t + gap;  // 14.5mm

// Component positions — centered in the inner cavity
bat_x = wall + (cav_inner_w - bat_w) / 2;
bat_y = wall + tolerance;
esp32_x = wall + (cav_inner_w - esp32_w) / 2;
esp32_y = bat_y + bat_h + gap;
esp32_z = cavity_z + (bat_t - esp32_t) / 2;

// USB cutout (top edge of box, centered on ESP32)
usb_cut_x = esp32_x + (esp32_w - esp32_usb_w) / 2;
usb_cut_y = outer_h - wall;
usb_cut_z = esp32_z;

// Pin hole positions (in the recess floor, matching bezel pins)
// Bezel is centered in recess, pins relative to bezel
pin_recess_off_x = (recess_w - bezel_w) / 2;
pin_recess_off_y = (recess_h - bezel_h) / 2;

echo(str("Box: ", outer_w, " x ", outer_h, " x ", box_depth, " mm"));
echo(str("Cavity inner: ", cav_inner_w, " x ", cav_inner_h, " mm"));
echo(str("Top recess: ", recess_w, " x ", recess_h, " x ", recess_depth, " mm (centered)"));
echo(str("Bezel: ", bezel_w, " x ", bezel_h, " x ", bezel_t, " mm"));
echo(str("  Display window: ", b_win_w, "x", b_win_h, " at (", b_win_x, ", ", b_win_y, ")"));
echo(str("  Frame left=", b_win_x, " right=", bezel_w - b_win_x - b_win_w, " top=", b_win_y, " bottom=", bezel_h - b_win_y - b_win_h));
echo(str("PCB fully wrapped by ", bezel_inset, "mm walls on all sides"));

/* [Ventilation] */
vent_slots = 8;  vent_w = 2.5;  vent_h = 40.0;
/* [Wall Mount] */
mount_hole_d = 5.0;  mount_hole_off = 12.0;
/* [Printer] */
layer_h = 0.2;

// ─── MODULES ───

module bezel() {
    difference() {
        cube([bezel_w, bezel_h, bezel_t]);
        // Display window (centered)
        translate([b_win_x, b_win_y, -0.1])
            cube([b_win_w, b_win_h, bezel_t + 0.2]);
        // Screw clearance holes (matching PCB)
        for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
            for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
                translate([x, bezel_h - y, -0.1])
                    cylinder(d = hole_d + 0.2, h = bezel_t + 0.3, $fn = 16);
            }
        }
        // Countersink for screw heads
        for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
            for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
                translate([x, bezel_h - y, bezel_t - 0.5])
                    cylinder(d1 = hole_d + 3, d2 = hole_d + 0.2, h = 0.6, $fn = 16);
            }
        }
    }
    // Alignment pins (protrude from bezel back, through PCB, into box)
    for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
        for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
            translate([x, bezel_h - y, 0])
                cylinder(d = pin_d - 0.2, h = pin_h, $fn = 16);
        }
    }
}

module box() {
    difference() {
        // Outer shell
        cube([outer_w, outer_h, box_depth]);
        
        // Main cavity (inner box, larger than PCB)
        translate([wall, wall, cavity_z])
            cube([cav_inner_w, cav_inner_h, box_depth - cavity_z + 1]);
        
        // Top recess — bezel+PCB drops in here
        translate([recess_x, recess_y, wall - 0.1])
            cube([recess_w, recess_h, recess_depth + 0.2]);
        
        // Glass recess — tiny step deeper within the recess for the display glass
        gx = recess_x + (recess_w - disp_w) / 2;
        gy = recess_y + (recess_h - disp_h) / 2;
        translate([gx, gy, wall + bezel_t - glass_recess - 0.1])
            cube([disp_w, disp_h, glass_recess + 0.2]);
        
        // Pin holes in recess floor
        for (x = [hole_off_x, epd_pcb_w - hole_off_x]) {
            for (y = [hole_off_y_bot, epd_pcb_h - hole_off_y_top]) {
                px = recess_x + pin_recess_off_x + x;
                py = recess_y + pin_recess_off_y + (bezel_h - y);
                translate([px, py, wall - 0.1])
                    cylinder(d = pin_d, h = recess_depth + 1, $fn = 16);
            }
        }
        
        // USB cutout (top edge)
        translate([usb_cut_x, usb_cut_y - 0.5, usb_cut_z])
            cube([esp32_usb_w, wall + 1.5, esp32_usb_h]);
        
        // Battery pocket
        translate([bat_x, bat_y, cavity_z])
            cube([bat_w + tolerance, bat_h + tolerance, bat_t + gap]);
        
        // ESP32 pocket
        translate([esp32_x, esp32_y, esp32_z])
            cube([esp32_w + tolerance, esp32_h + tolerance, esp32_t + gap]);
        
        // Ventilation slots (back wall)
        slot_spacing = (cav_inner_w - 20) / (vent_slots + 1);
        for (i = [1:vent_slots]) {
            sx = wall + 10 + i * slot_spacing;
            translate([sx, wall + 15, box_depth - wall - 0.1])
                cube([vent_w, cav_inner_h - 30, wall + 0.3]);
        }
        
        // Wall-mount holes (back wall)
        for (x = [mount_hole_off, outer_w - mount_hole_off]) {
            for (y = [mount_hole_off, outer_h - mount_hole_off]) {
                translate([x, y, box_depth - wall - 0.1])
                    cylinder(d = mount_hole_d, h = wall + 0.3, $fn = 16);
                translate([x, y, box_depth - wall - 0.5])
                    cylinder(d1 = mount_hole_d, d2 = mount_hole_d + 3, h = 1.0, $fn = 16);
            }
        }
    }
    
    // Kiko's Rose engraving on inside of back wall
    rose_size = 18.0;
    rx = outer_w / 2;
    ry = outer_h / 2 + 10;
    for (i = [0:4]) {
        a = i * 72;
        dx = (rose_size / 3.5) * cos(a);
        dy = (rose_size / 3.5) * sin(a);
        translate([rx + dx, ry + dy, box_depth - 0.4])
            rotate([0, 0, a + 15])
                scale([1.0, 0.45])
                    cylinder(d = (rose_size / 3.5) * 1.4, h = 0.5, $fn = 24);
        translate([rx + dx * 0.55, ry + dy * 0.55, box_depth - 0.4])
            rotate([0, 0, a - 10])
                scale([1.0, 0.35])
                    cylinder(d = (rose_size / 3.5) * 0.9, h = 0.5, $fn = 20);
    }
    translate([rx, ry, box_depth - 0.4])
        cylinder(d = 2.5, h = 0.5, $fn = 12);
}

// ─── RENDER ───

// For box-only: comment out the bezel line below; uncomment "box()"
box();

// For bezel-only: uncomment this, comment out "box()" above
// translate([0, outer_h + 10, 0]) bezel();

echo("========================================");
echo(str("  Box:  ", outer_w, " x ", outer_h, " x ", box_depth, " mm"));
echo(str("  Bezel: ", bezel_w, " x ", bezel_h, " x ", bezel_t, " mm"));
echo(str("  PCB fully wrapped by ", bezel_inset, "mm walls on all 4 sides"));
echo(str("  Print: box back-face DOWN, bezel flat, no supports"));
echo(str("  ~", ceil(box_depth / layer_h), " / ", ceil(bezel_t / layer_h), " layers"));