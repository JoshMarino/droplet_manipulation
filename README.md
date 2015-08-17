Droplet Manipulation
=============================================

Josh Marino 
---------------------------------------------


![period_2_bounce](https://raw.githubusercontent.com/JoshMarino/droplet_manipulation/master/period_2_bouncing.png)


## Overview

The intention of this project was to study the simultaneous levitation and manipulation of droplets. It has been researched and proven that droplets can bounce on a thin layer of air created by vibrating a liquid, typically silicon oil. The properties of vibrating the bath of silicon oil (1000 cSt) were frequencies in the range of 40 - 200 Hz with a bath acceleration of 0.7 - 5.0 G's. Droplets have also been silicon oil with a kinematic viscosity of 10-100 cSt and diameter of 0.4 - 1.0 mm. 

The first step in simultaneous levitation and manipulation of droplets using the [6-DOF PPOD (Programmable Part-feeding Oscillatory Devices)](http://nxr.northwestern.edu/research/robotic-manipulation) was finding conditions in which droplets were able to stably bounce at lower frequencies, 10 - 50 Hz, due to the capabilities of the PPOD. Once those conditions were found, the next step of this project was starting to make progress in the horizontal transportation of the droplets, which has not been studied up to this point. Horizontal motion of the droplets was produced by not only shaking the PPOD in the vertical direction, but also horizontal vibration. This has not been studied before due to others only using a 1-DOF shaker.


### Spring Quarter

The first few weeks of the Spring quarter was being introduced to the project, PPOD robotic platform, and reading some papers of related work. Once familiar with the previous work, a method in which drops were produced needed to be developed. Some ideas were using a syringe pump, piezoelectric generation, and linear motor attached to syringe. It was decided to go with a [micro linear actuator](http://www.firgelli.com/pdf/L12_datasheet.pdf) purchased from Firgelli. The decision was not made for the syringe pump or piezoelectric generator due to high velocity drops being generated from the piezoelectric device, which could cause problems with coalescence, and due to the high price range for a syringe pump. Once the linear actuator arrived with the LAC (linear actuator control) board, initial testing was performed to ensure proper functioning. First, a potentiometer was hooked up to the LAC and the position was controlled by the rotation of the potentiometer. Next, simple C code was created that controlled the position of the linear actuator through a PWM signal, which incremented its duty cycle a certain amount each time a button was pressed, in order to create a drop. Unfortunately, this C code did not perform well with the LAC, and it was decided to use a [GUI](http://firgelli.com/Uploads/LAC%20Advanced%20Configuration.pdf) provided with the LAC.

Once the linear actuator, control board, and GUI were working together, a platform had to be attached to the PPOD superstructure in order to dispense drops in a container sitting on the PPOD platform. This was made with 80/20 10 Series steel and attached to the PPOD superstructure in a manner such that the position above the container and angle of it could be controlled. This would later be necessary to determine an ideal impact velocity. After the structure was mounted on the PPOD, the linear actuator was attached using a C-shaped clamp. Next, a syringe had to be added to the structure that was collinear with the linear actuator. This was fabricated with parts made from a laser cutter and attached to the structure with screws. The syringe assembly was then tested to ensure everything worked the way it was supposed to and that drops were able to be produced.

Before being able to testing drops vertically bouncing, the container sitting on the PPOD platform needed to be remade. The current container slowly leaked and did not have enough surface area for later horizontal movement of the drops. It turned out a similar platform was found in another lab of one of my professor's in which we used. The only modification was attaching a magnet to the underside of the platform. This was necessary to ensure the platform did not move relative to the PPOD platform; a magnet was also attached underneath the PPOD platform.

### Summer Quarter

Also crucial to the bouncing drop testing was being able to view the dynamics of the drop. A high-speed camera, [PhotonFocus TrackCam](http://www.stemmer-imaging.fi/media/uploads/docmanager/12782-Photonfocus_MV-D1024_Trackcam_Manual.pdf), was used for this, which was already existing in the NxR laboratory. However, the parts for interfacing the camera with a computer had to be located and installed on a computer sitting close to the PPOD. These parts included a MicroEnable III PCI card, CameraLink cable (8-10 ft), power supply, lens, GUI from PhotonFocus to set the camera properties, and a frame grabber that was installed via CD for the MicroEnable III. The only missing piece from the NxR lab was the power supply, in which an existing 5 V / 1 A power brick was attached to a Series 712: Male plug purchased from Binder-USA.

After the major components from the Spring quarter were put into place, testing of vertical bouncing droplets started. The parameters that were available for testing were: (1) silicon oil of different viscosities (20, 100, 1000 mPa) for both the bath and droplets, (2) vibration frequency (14, 20, 26, 32, 38, 44, 50 Hz), (3) vibration amplitude (0.5 - 2.0 G), (4) diameter of drops produced, and (5) impact velocity of drops. Numerous tests were first run before gathering any data to observe trends in order to reduce the sampling size of the data. Noticed from these initial tests was that the smallest producable drops were best for stable bouncing, as well as the higher range of impact velocities caused instant coalescenses. Using these findings, a series of experiments were conducted to determine the stability of vertically bouncing drops for either being stable or unstable; any drops lasting less than 3 minutes was considered unstable bouncing. Plots were then created showing the results of the vibration frequency and amplitude for either producing stable or unstable drops.

![Stability_100_100](https://raw.githubusercontent.com/JoshMarino/droplet_manipulation/master/Vertical%20Bouncing%20Stability%20Plots/Stability_100_100.jpg)

![Stability_100_1000](https://raw.githubusercontent.com/JoshMarino/droplet_manipulation/master/Vertical%20Bouncing%20Stability%20Plots/Stability_100_1000.jpg)

![Stability_20_1000](https://raw.githubusercontent.com/JoshMarino/droplet_manipulation/master/Vertical%20Bouncing%20Stability%20Plots/Stability_20_1000.jpg)
