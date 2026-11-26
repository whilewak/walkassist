// WALKASSIST - electronics box that zip-ties onto the cane shaft, just below the handle.
//
// Parts (set `part` below or use the Customizer):
//   box        - holds the perf board (Nano + drivers) and the 9 V battery, which lies
//                flat (fat side down) across the bottom of the box, long side across
//                the cane, to keep the box shallow. Its back is a half-round cradle that
//                sits on the shaft. Each zip tie goes straight up through a slot beside
//                the shaft into the box, across a groove in the inside floor (visible),
//                down a slot on the other side, and around the shaft.
//   lid        - sliding lid. It slides down into angled (dovetail) grooves in the
//                side walls from the top end (handle side) and stops on the bottom wall.
//                Gravity keeps it closed with the cane upright; a small bump on its
//                inside clicks past the top wall so it stays shut when the cane lies
//                flat or is dropped. Buzzer grille and (optionally) the LED hole.
//   fit_test   - the bottom 17 mm of the box (cradle + one zip-tie path): checks the
//                shaft fit and the zip-tie size
//   slide_test - the top 22 mm of the box + the top 22 mm of the lid: checks that the
//                lid slides smoothly and clicks shut. Print both tests FIRST.
//
// Hardware: 3 zip ties (default sized for 4.8 mm wide ties), a thin rubber strip
//           (bike inner tube) between shaft and cradle, double-sided foam tape for
//           the perf board (it also keeps the solder joints off the floor).
//           No screws, heat-set inserts, nuts or straps.
//
// Mounting (lid off, before the perf board and battery go in):
//   1. Rubber strip on the shaft, box cradle on top.
//   2. For each tie: push it up through one slot from the shaft side, lay it in the
//      floor groove, push it down the other slot, wrap it around the front of the
//      shaft and lock it. Pull tight and trim.
//   3. Then fit the perf board and battery; they sit on top of the flush ties.
//   4. Slide the lid down from the top end until it clicks.
//
// Coordinates: the shaft runs along Z through the origin. The box sits on +Y.
// Z = 0 is the bottom end of the box (toward the cane tip).

/* [What to render] */
part = "assembly"; // [assembly, box, lid, fit_test, slide_test, print_all]

/* [Measure: cane] */
shaft_d = 22.0;        // shaft outer diameter where the box goes (calipers)
liner_t = 1.0;         // rubber strip thickness under the cradle (0 = none)

/* [Measure: zip ties] */
tie_w = 4.8;           // zip tie width
tie_t = 1.4;           // zip tie thickness
tie_count = 3;         // ties along the box (2 or 3)
tie_margin = 10;       // first/last tie centre, mm from the box ends

/* [Measure: perf board] */
pcb_w = 50;            // width (across the shaft): fits the 52 mm inside width
pcb_l = 50;            // length (along the shaft): board cut down to 50 mm
pcb_t = 1.6;
board_gap = 3;         // foam tape under the board (keeps solder joints off the floor)
parts_h = 10;          // tallest part above the board (Nano soldered on its pins)  ESTIMATE
usb_above_pcb = 5.5;   // Nano USB port centre, height above the perf board top  ESTIMATE
usb_x = 0;             // USB port centre, sideways offset from box centre

/* [Measure: battery] */
// The 9 V battery lies flat (fat side down) across the bottom of the box: its length
// runs across the cane (X), its width along the cane (Z), its thickness up from the floor (Y).
batt_l = 48.5;         // length (across the box)
batt_w = 26.5;         // width (along the cane)
batt_t = 17.5;         // thickness (up from the floor): sets the box depth
batt_clip = 0;         // extra length if the snap clip sticks out past the battery's end
                       // (0: the battery fits across the 52 mm inside as in the test print)

/* [Measure: LED, buzzer, wires] */
led_face = "top";      // [lid, top]
led_d = 5.3;           // 5 mm LED
led_x = -14;
led_z = 95;            // only used when led_face = "lid"
buzzer_x = 12;         // buzzer centre (grille in the lid)
buzzer_z = 70;
wire_hole_d = 6;
grip_wire_x = 14;      // hole in the top end for the grip (FSR) wires
cable_x = -14;         // hole in the bottom end for the tip IMU cable

/* [Fit and print] */
fit = 0.4;             // extra cradle clearance (diameter)
cradle_y0 = 1.0;       // cradle edge sits this far past the shaft centre, so the
                       // ties pull the box tight instead of the cradle bottoming out
wall = 2.4;
tie_clear = 1.0;       // slot clearance around the tie
lid_t = 2.4;           // lid thickness
dovetail_angle = 30;   // lid edge slope, degrees from straight up (<= 45 prints without supports)
slide_clear = 0.3;     // sideways gap per edge in the groove: bigger = looser slide
detent_h = 0.5;        // how far the click bump sticks out: 0 = no click
min_inner_d = 20;      // inside depth (floor to lid) is at least this, for extra headroom

$fn = 72;

// ------------------------------------------------------------------ derived --
bore_r    = shaft_d / 2 + liner_t + fit / 2;
slot_t    = tie_t + tie_clear;                // zip-tie slot, across the shaft (X)
slot_w    = tie_w + tie_clear;                // zip-tie slot, along the shaft (Z)
slot_x_in = bore_r + 2.0;                     // slot inner edge: 2 mm of cradle beside the shaft
slot_x_out = slot_x_in + slot_t;
groove_d  = tie_t + 0.3;                      // floor groove depth, so the tie sits flush
divider_t = 2;
slide_depth = lid_t * tan(dovetail_angle);    // groove depth into each side wall; sized so the
                                              // front opening stays the full inside width
side_wall = wall + slide_depth;               // side walls hold the lid grooves
inner_w   = max(pcb_w, batt_l + batt_clip) + 2;
inner_h   = batt_w + 0.5 + divider_t + 1 + pcb_l + 1;
inner_d   = max(max(board_gap + pcb_t + parts_h,                    // tallest part on the board
                    batt_t,                                         // battery lying flat
                    board_gap + pcb_t + usb_above_pcb + 4.5)        // USB plug
                + 1,                                                // clearance
                min_inner_d);                                       // usable depth under the lid
outer_w   = inner_w + 2 * side_wall;
outer_h   = inner_h + 2 * wall;
back_y    = bore_r + 1.5;                     // outer face of the back wall
in_y0     = back_y + wall;                    // inner face of the back wall (inside floor)
lid_y0    = in_y0 + inner_d;                  // inner face of the lid
front_y   = lid_y0 + lid_t;                   // front face of the box (lid sits flush)
cradle_hw = slot_x_out + 2.5;                 // cradle half-width where it meets the shaft
batt_z1   = wall + batt_w + 0.5;              // top of the battery bay (divider starts here)
pcb_z0    = batt_z1 + divider_t + 1;          // perf board bottom edge
usb_y     = in_y0 + board_gap + pcb_t + usb_above_pcb;
tie_zs    = tie_count == 2 ? [tie_margin, outer_h - tie_margin]
                           : [tie_margin, outer_h / 2, outer_h - tie_margin];
slide_test_z = outer_h - 22;                  // where the slide test pieces start

echo(str("Box outside: ", outer_w, " wide x ", outer_h, " long x ",
         front_y - cradle_y0, " deep (mm)"));
echo(str("Cradle fits a ", shaft_d, " mm shaft + ", liner_t, " mm liner (bore ", 2 * bore_r,
         " mm); slots take ", tie_w, " x ", tie_t, " mm zip ties"));
echo(str("Floor under each tie groove, above the shaft: ", in_y0 - groove_d - bore_r, " mm"));
echo(str("Battery top to lid: ", inner_d - batt_t, " mm"));
echo(str("Tallest part allowed on the perf board: ", inner_d - board_gap - pcb_t, " mm"));

// ------------------------------------------------------------------- helpers --
// Cylinder along +Y (rotate maps +Z to +Y).
module cyl_y(d, h) { rotate([-90, 0, 0]) cylinder(d = d, h = h); }

// Zip-tie path: two straight slots beside the shaft, from the cradle face up
// into the box, joined by a groove across the inside floor. Straight slots
// thread easily; the tie only bends inside the open box, where you can guide it.
module tie_path(zc) {
  translate([0, 0, zc - slot_w / 2]) {
    for (s = [-1, 1])
      translate([s > 0 ? slot_x_in : -slot_x_out, cradle_y0 - 1, 0])
        cube([slot_t, in_y0 - cradle_y0 + 2, slot_w]);
    translate([-slot_x_out, in_y0 - groove_d, 0]) cube([2 * slot_x_out, groove_d + 1, slot_w]);
  }
}

// Space the lid slides in: a dovetail (wider at the lid's inner face, sloped
// sides) from the top of the bottom wall up through the top end.
module lid_track() {
  hw_in  = inner_w / 2 + slide_depth;         // half-width at the lid's inner face
  hw_out = hw_in - slide_depth;               // half-width at the front = inside width
  translate([0, 0, wall])
    linear_extrude(height = outer_h)
      polygon([[-hw_in, lid_y0], [hw_in, lid_y0], [hw_out, front_y],
               [hw_out, front_y + 1], [-hw_out, front_y + 1], [-hw_out, front_y]]);
}

// ---------------------------------------------------------------------- box --
module box() {
  difference() {
    union() {
      // shell
      translate([-outer_w / 2, back_y, 0]) cube([outer_w, front_y - back_y, outer_h]);
      // sloped cradle from the shaft to the back wall (prints without supports)
      hull() {
        translate([-cradle_hw, cradle_y0, 0]) cube([2 * cradle_hw, 0.01, outer_h]);
        translate([-outer_w / 2, back_y, 0]) cube([outer_w, 0.01, outer_h]);
      }
    }

    // inside
    translate([-inner_w / 2, in_y0, wall]) cube([inner_w, inner_d + 1, inner_h]);
    // lid track (grooves in the side walls + open slot through the top end)
    lid_track();
    // shaft
    translate([0, 0, -1]) cylinder(r = bore_r, h = outer_h + 2);
    // zip-tie slots + floor grooves
    for (zc = tie_zs) tie_path(zc);

    // top end: USB port, grip wires, LED (optional)
    translate([usb_x - 6.5, usb_y - 4.5, outer_h - wall - 1]) cube([13, 9, wall + 2]);
    translate([grip_wire_x, in_y0 + inner_d * 0.55, outer_h - wall - 1])
      cylinder(d = wire_hole_d, h = wall + 2);
    if (led_face == "top")
      translate([led_x, in_y0 + inner_d * 0.55, outer_h - wall - 1]) cylinder(d = led_d, h = wall + 2);
    // bottom end: tip IMU cable
    translate([cable_x, in_y0 + inner_d * 0.55, -1]) cylinder(d = wire_hole_d, h = wall + 2);
  }

  // (no perf board standoffs: the board sits on foam tape on the floor)

  // divider between battery bay and electronics
  translate([-inner_w / 2, in_y0 - 0.01, batt_z1]) cube([inner_w, batt_t * 0.6, divider_t]);
}

// ---------------------------------------------------------------------- lid --
// Dovetail plate, slightly smaller than the track. The click bump on its inner
// face, near the top, has to be pushed past the top wall's inside edge.
module lid() {
  t      = lid_t - 0.2;                       // a little thinner than the track
  hw_in  = inner_w / 2 + slide_depth - slide_clear;
  hw_out = hw_in - t * tan(dovetail_angle);
  z0     = wall + 0.2;                        // rests on the bottom wall
  difference() {
    union() {
      translate([0, 0, z0])
        linear_extrude(height = outer_h - z0)
          polygon([[-hw_in, lid_y0], [hw_in, lid_y0], [hw_out, lid_y0 + t], [-hw_out, lid_y0 + t]]);
      // click bump: a short ridge across the inner face, just below the top wall
      if (detent_h > 0)
        // (r = 1.2 so no corner of the 72-sided cylinder lands exactly on the
        //  lid surface, which would leave a zero-thickness seam in the mesh)
        translate([0, lid_y0 + 1.2 - detent_h, outer_h - wall - 1.5])
          rotate([0, 90, 0]) cylinder(r = 1.2, h = 10, center = true);
    }
    // buzzer grille
    translate([buzzer_x, lid_y0 - 1, buzzer_z]) cyl_y(2.5, lid_t + 2);
    for (a = [0 : 60 : 300])
      translate([buzzer_x + 4.5 * cos(a), lid_y0 - 1, buzzer_z + 4.5 * sin(a)]) cyl_y(2, lid_t + 2);
    if (led_face == "lid")
      translate([led_x, lid_y0 - 1, led_z]) cyl_y(led_d, lid_t + 2);
    // finger grip near the top edge, to push the lid up and out
    for (k = [0 : 2])
      translate([-8, lid_y0 + t - 0.6, outer_h - 4 - 2.5 * k]) cube([16, 1, 1.2]);
  }
}

// ---------------------------------------------------------------- fit tests --
module fit_test() {
  intersection() {
    box();
    translate([-100, -100, 0]) cube([200, 200, tie_margin + slot_w / 2 + 4]);
  }
}

module above(z) { intersection() { children(); translate([-100, -100, z]) cube([200, 200, 300]); } }

// ------------------------------------------------------------ print layouts --
// Each part turned so its flat face is on the bed and nothing needs supports.
module on_bed_box() { translate([0, 0, -cradle_y0]) rotate([90, 0, 0]) children(); }
module on_bed_lid() { translate([0, 0, lid_y0 + lid_t - 0.2]) rotate([-90, 0, 0]) children(); }

module assembly() {
  color("silver") translate([0, 0, -60]) cylinder(d = shaft_d, h = outer_h + 140);
  color("SteelBlue") box();
  color("LightSteelBlue", 0.6) lid();
  // zip ties: across the floor groove, down both slots, around the front of the shaft
  tx = slot_x_in + slot_t / 2 - tie_t / 2;           // tie's inner edge in the slot
  for (zc = tie_zs)
    color("Orange") translate([0, 0, zc - tie_w / 2]) {
      translate([-tx - tie_t, in_y0 - groove_d, 0]) cube([2 * (tx + tie_t), tie_t, tie_w]);
      for (s = [-1, 1])
        translate([s > 0 ? tx : -tx - tie_t, 0, 0]) cube([tie_t, in_y0 - groove_d + tie_t, tie_w]);
      intersection() {
        difference() {
          cylinder(r = tx + tie_t, h = tie_w);
          translate([0, 0, -1]) cylinder(r = tx, h = tie_w + 2);
        }
        translate([-50, -50, -1]) cube([100, 50, tie_w + 2]);
      }
    }
}

if (part == "assembly") assembly();
else if (part == "box") on_bed_box() box();
else if (part == "lid") on_bed_lid() lid();
else if (part == "fit_test") on_bed_box() fit_test();
else if (part == "slide_test") {
  translate([0, outer_h, 0]) on_bed_box() above(slide_test_z) box();
  translate([outer_w + 10, -slide_test_z, 0]) on_bed_lid() above(slide_test_z) lid();
}
else if (part == "print_all") {
  on_bed_box() box();
  translate([outer_w + 10, 0, 0]) on_bed_lid() lid();
}
