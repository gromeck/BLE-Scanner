/*
    case for the BLE-Scanner device

	(c) 2020 by Christian.Lorenz@gromeck.de


	This file is part of BLE-Scanner

    BLE-Scanneris free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BLE-Scanneris distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BLE-Scanner  If not, see <https://www.gnu.org/licenses/>.

*/

// fragment control to make curves smooth
$fa = 6;
$fs = 0.1;
$fn = 0;

// generate the final output
final = true;
//final = false;

generate_pcb = (final) ? false : true;
generate_case_closed = (final) ? false : true;
generate_strap = true;

// the gap used to place elements
gap = 5;

// slackness control
slackness = 0.2;
slackness_snap = slackness * 1;
slackness_pcb = slackness * 3;
slackness_usb = slackness * 4;
slackness_reset = slackness;
case_snap_height = 0.5;

// PCB board dimension
pcb_length = 39.0;
pcb_width = 31.4;
pcb_height = 4.75;
pcb_board_height = 1.6;

// usbport cutout
usbport_width = 8.0;
usbport_height = 2 * 2.5;
usbport_edge_radius = 0.5;
usbport_offset_z = 1.25;

// usbport cutout
reset_width = 4.5;
reset_height = 2.0;
reset_edge_radius = 0.5;
reset_offset_x = 1;
reset_offset_z = 1.25;

// the dimension of the case
case_thickness = 1.2;
case_edge_radius_rel = case_thickness;
case_edge_radius = case_thickness * case_edge_radius_rel;
case_length = pcb_length + 2 * case_thickness + slackness_pcb;
case_width = pcb_width + 2 * case_thickness + slackness_pcb;
case_lower_height = case_thickness + pcb_height + 2 * slackness;

// the snap dimensions
case_snap_width = case_length * 3 / 4 * 4 / 3 * 1 / 2;
case_snap_depth = case_width * 3 / 4 * 4 / 3 * 1 / 2;
case_snap_block_height = case_lower_height - pcb_board_height - case_thickness;
case_snap_offset_z = (case_snap_block_height - 2 * case_snap_height) / 2;

// the mounting strap
strap_thickness = case_thickness * 1.5;
strap_edge_radius = 2 * case_thickness;
strap_hole_radius = 3.0/2 + slackness;
strap_length = strap_hole_radius * 2 * 2 * 1.1;
strap_width = strap_length;

echo("case length=",case_length);
echo("case width=",case_width);
echo("case height=",case_lower_height + case_thickness);

// LED
led_hole_length = 5.0;
led_hole_width = 2.0;
led_hole_radius = 0.8;
led_offset_x = pcb_length - led_hole_length - 1;
led_offset_y = 2;

// logo
logo_file = "../Ressources/Bluetooth-Simple-Logo.svg";
logo_thickness = 0.2;
logo_offset_x = case_length / 3 * 2;
logo_offset_y = case_width / 2;
logo_dpi = 120;

// PCB
pcb_offset_x = 0;
pcb_offset_y = (case_width - 2 * case_thickness - pcb_width) / 2;
pcb_file = "../Ressources/ESP32_D1_Mini.stl";


module make_rounded_square(length,width,thickness,roundedfront = false,roundedback = false)
{
    linear_extrude(thickness) {
        if (roundedfront)
            translate([width/2,0,0])
                circle(width / 2);
        if (roundedback)
            translate([length - width / 2,0,0])
                circle(width / 2);

        translate([roundedfront ? width / 2 : 0,-width / 2,0])
            square([
                length - (roundedfront ? width / 2 : 0) - (roundedback ? width / 2 : 0),
                width],center = false);
    }
}

module make_rounded_box(height,width,depth,radius = 0)
{
    linear_extrude(height)
        if (radius)
            minkowski() {
                translate([radius,radius,0])
                    square([
                        width - 2 * radius,
                        depth - 2 * radius
                        ],center = false);
                circle(radius);
            }
        else
            square([width,height]);
}

/*
**  make a prism
*/
module make_prism(length,width,height,draft = 0)
{
    polyhedron(
        points = [
            [0,0,0],
            [length,0,0],
            [length,width,0],
            [0,width,0],
            [draft,width / 2,height],
            [length - draft,width / 2,height] ],
        faces=[
            [0,1,2,3],
            [5,4,3,2],
            [0,4,5,1],
            [0,3,4],
            [5,2,1]]
           );
}

/*
**  build the PCB components
*/
module make_pcb()
{
    color("blue")
        translate([case_thickness + pcb_offset_x,case_thickness + pcb_offset_y,case_thickness])
            translate([pcb_length / 2 + 1.5,pcb_width / 2,-pcb_height / 2 + 0.4]) // the STL is not really aligned
                rotate([0,0,-90])
                    import(pcb_file,1);
}

/*
** build one generic half shell
**
**  height       is the total net height of the shell
**  snap_border  is the negative or positive height of the inner snap border
*/
module make_case_half_shell(height,snap_border = 0)
{
    difference() {
        union() {
            // the case itself
            make_rounded_box(height,case_length,case_width,case_edge_radius);

            // the snap border
            if (snap_border > 0)
                translate([case_thickness / 2,case_thickness / 2,height])
                    make_rounded_box(snap_border,case_length - case_thickness,case_width - case_thickness,case_edge_radius);
        }

        // make the case hollow
        if (height > case_thickness) {
            translate([case_thickness,case_thickness,case_thickness])
                make_rounded_box(
                        height + abs(snap_border),
                        case_length - 2 * case_thickness,
                        case_width - 2 * case_thickness,
                        case_edge_radius);
            if (snap_border < 0)
                translate([case_thickness / 2 + slackness_snap / 2,case_thickness / 2 + slackness_snap / 2,height - abs(snap_border) + slackness_snap])
                    make_rounded_box(
                        abs(snap_border),
                        case_length - case_thickness - slackness_snap,
                        case_width - case_thickness - slackness_snap,
                        case_edge_radius);
        }
    }
}

/*
** build one generic half shell
**
**  height       is the total net height of the shell
**  snap_border  is the negative or positive height of the inner snap border
*/
module make_strap()
{
	translate([-strap_length,0,0])
		difference() {
			union() {
				// usbport side
				for (side = [-1,+1])
					translate([0,case_width / 2  - strap_width / 2 + side * (case_width - strap_width) / 2,0])
						make_rounded_box(strap_thickness,strap_length * 2,strap_width,strap_edge_radius);

				// non-usbport side
				translate([case_length,case_width / 2  - strap_width / 2,0])
					make_rounded_box(strap_thickness,strap_length * 2,strap_width,strap_edge_radius);
			}

			// cut out the case
			translate([strap_length,0,0])
				make_rounded_box(case_lower_height,case_length,case_width,case_edge_radius);

			// drill the holes
			# if (0) translate([(case_length + 2 * strap_length) / 2,case_width / 2,0])
				for (side = [-1,+1])
					for (corner = [-1,+1])
						translate([side * ((case_length + 2 * strap_length) / 2 - (strap_length / 2)),corner * (case_width / 2 - (strap_width / 2)),0])
							cylinder(h = strap_thickness,r = strap_hole_radius);

			translate([(case_length + 2 * strap_length) / 2,case_width / 2,0])
				for (side = [-1,+1])
					translate([-case_length / 2 - strap_length / 2,side * (case_width / 2 - (strap_width / 2)),0])
							cylinder(h = strap_thickness,r = strap_hole_radius);

			translate([strap_length + case_length + strap_length / 2,case_width / 2,0])
				cylinder(h = strap_thickness,r = strap_hole_radius);

		}
}

/*
** build the lower part of the case
*/
module make_case_lower_shell()
{
    difference() {
        // create the shell itself
        make_case_half_shell(case_lower_height,0);

        // cut out the inner once more -- but as square
        translate([case_thickness,case_thickness,case_thickness])
            cube([case_length - 2 * case_thickness,case_width - 2 * case_thickness,case_lower_height]);

        // cut out the USB connector
        translate([-case_thickness / 2,case_width / 2 - usbport_width / 2 - slackness_usb / 2,case_thickness + usbport_offset_z])
            rotate([90,0,90])
                make_rounded_box(case_thickness * 2,usbport_width + slackness_usb,usbport_height + slackness_usb,usbport_edge_radius);

        // cut out the reset button
        translate([case_thickness + reset_offset_x,case_width + case_thickness / 2,case_thickness + reset_offset_z])
            rotate([90,0,0])
                make_rounded_box(case_thickness * 2,reset_width + slackness_reset,reset_height + slackness_reset,reset_edge_radius);

        // cut out the snaps on the long sides
        for (side = [-1,+1])
            translate([(case_length - case_snap_width) / 2,case_width / 2 + side * (case_width / 2 - case_thickness),case_lower_height - case_snap_height - case_snap_offset_z])
                rotate([90 * -side,0,0])
                    translate([0,-case_snap_height,0])
                        make_prism(case_snap_width,case_snap_height * 2,case_snap_height,case_snap_height);

        // cut out some help to open the case
        for (side = [-1,+1])
            translate([case_length,case_width / 2 + side * case_width / 2,case_lower_height])
                cylinder(h = case_thickness * 2,r = case_edge_radius,center = true);
    }

    // add the PCB
    if (generate_pcb)
        make_pcb();
}

/*
**  build the upper part of the case
*/
module make_case_upper_shell()
{
    difference() {
        // create the shell itself
        make_case_half_shell(case_thickness,0);

        // cut out the led
        translate([case_thickness + led_offset_x,case_thickness + led_offset_y,-case_thickness / 2])
            make_rounded_box(case_thickness * 2,led_hole_length,led_hole_width,led_hole_radius);

        // cut out the bluetooth logo
        translate([logo_offset_x,logo_offset_y,-logo_thickness])
            rotate([0,0,-90])
                linear_extrude(logo_thickness * 2)
                    import(logo_file, center = true, dpi = logo_dpi);
    }

    // add barriers on short sides
    difference() {
        union() {
            for (side = [-1,+1])
                translate([case_length / 2 + side * (case_length / 2- case_thickness * 3 / 2 - slackness_snap),3 * case_edge_radius,case_thickness])
                    rotate([0,0,90])
                        translate([0,-case_thickness / 2,0])
                            make_rounded_box(case_thickness,1 * case_width - 2 * 3 * case_edge_radius,case_thickness,case_thickness / 4);
        }

        // cut out the usb port
        translate([case_length - case_thickness,2 * case_edge_radius,case_thickness])
            rotate([0,0,90])
                translate([case_width / 2/2,-case_thickness / 2 * 2,0])
                    cube([usbport_width + slackness_usb,case_thickness * 3,case_thickness * 2]);
    }

    // add snap counterparts on long sides
    for (side = [-1,+1])
        union() {
            // the block
            translate([
                    (case_length - case_snap_width * 1.1) / 2,
                    case_width / 2 - case_thickness / 2 + side * (case_width / 2 - case_thickness * 3 / 2- slackness_snap),
                    case_thickness])
                make_rounded_box(case_snap_block_height,case_snap_width * 1.1,case_thickness,case_snap_height / 2);

            // ... and add the snap
            translate([
                    (case_length - case_snap_width * 1.0) / 2,
                    case_width / 2 + side * (case_width / 2 - case_thickness - slackness_snap),
                    case_thickness + case_snap_height + case_snap_offset_z])
                rotate([90 * -side,0,0])
                    translate([0,-case_snap_height,0])
                        make_prism(case_snap_width * 1.0,case_snap_height * 2,case_snap_height,case_snap_height);
        }
}

/*
** put everthing together side by side
*/
union()
{
	if (generate_strap)
		make_strap();

    make_case_lower_shell();

    if (generate_case_closed) {
        // close the case
        translate([case_length,0,case_lower_height + case_thickness])
            rotate([180,0,180])
                make_case_upper_shell();
    }
    else {
        // put the upper case shell beside
        translate([0,case_width + gap,0])
            make_case_upper_shell();
    }
}/**/
