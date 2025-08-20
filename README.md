# FANS Extended
[![Releases](https://img.shields.io/badge/release-v1.1-blue
)](https://github.com/Flying-Ad-Hoc-Networks/Extended-UDP/releases)

This repository provides a modular and scalable codebase for Ad-hoc message transfer among multiple nodes, implemented using NS-3. It also includes integration with ROS for testing real-world networking scenarios, particularly those involving UAVs. The stack was initially developed and tested on a Jetson Nano running Ubuntu 18.04 Bionic, and all associated installations are designed to ensure compatibility with Ubuntu 18.04 LTS.

Future developments will focus on enhancing support for Ubuntu 22.04 or later, with a specific emphasis on adapting the code to work with ROS 2 and integrating it with the NS-3 simulator.

## System Requirements:
* Ubuntu 18.04 LTS (Bionic)
* NS-3 Dev/3.40
* ROS Melodic
* MAVROS

## VS CODE installation
Install older version of VS Code
```bash
wget https://update.code.visualstudio.com/1.62.3/linux-deb-arm64/stable -O vscode_arm64_old.deb
sudo dpkg -i vscode_arm64_old.deb
sudo apt -f install
```

## NS-3 Installation
--Pending--

## ROS Installation
* Install ROS Melodic Morenia
* Official documentation can be found [here](https://wiki.ros.org/melodic/Installation/Ubuntu)

## Mavros Installation
* Create a new workspace and follow official installation instructions from [here](https://github.com/mavlink/mavros/blob/master/mavros/README.md#installation)
* For `libgeographiclib` failure, just install them separately before running `install_geographiclib_datasets.sh`.

## Creating Executable (mavad_main)
This section focusses on running your own ROS-NS-3 scripts by creating an executable.

### Create a ROS package 
Using NS-3 packages in your scripts requires you to have them inside the `ns-3-dev` or `ns-3.40` folder. However ROS is not sourced inside the `hardware` folder itself. To work around this problem, we create a ROS package inside the `ns-3-dev` or `ns-3.40` folder, and source NS-3 packages inside that by making changes to the package `CMakeLists.txt` file.
* Inside  `ns-3-dev` or `ns-3.40`, run:
  ```bash
    catkin_create_pkg ros_ns3 roscpp rospy std_msgs geometry_msgs mavros_msgs
  ```
*  Change the package `CMakeLists.txt` file by adding the following lines at the very bottom:
  ```cmake
    file(GLOB_RECURSE mavades ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)
    set(DONT_BUILD )
    set(libraries_to_link ${libwifi} ${libapplications} ${libolsr} ${libnetanim} ${libinternet} ${libflow-monitor} ${libcore} ${libmobility} ${libconfig-store} ${libstats} ${libcsma} ${libbridge})
    add_executable(mavad_main src/node_x.cc)
    target_link_libraries(mavad_main ${catkin_LIBRARIES} ${ns3-libs} ${ns3-contrib-libs})
  ```
* Here you have to replace `node_x.cc` with the name of your script, ideally inside `ros_ns3/src`. You can create your own script using terminal command:
  ```bash
    touch my_script.cc
  ```
* The executable file (`mavad_main`) has been initialized here and will be created once the package is built.

### Add ros package contents as build dependencies
* NS-3 won't build `ros_ns3` package if it is not added as a build dependency in NS-3. To do that, add the following line in the `CMakeLists.txt` inside `ns-3-dev` or `ns-3.40`, under `#Build test utils`:
  ```cmake
    add_subdirectory(ros_ns3)
  ```
* Next build NS-3 using:
  ```bash
  cd ~/hardware/ns-3-allinone && ./build.py
  ```
* Once the build is done, you can navigate to `~/hardware/ns-3-allinone/ns-3-dev/cmake-cache/devel/lib/ros_ns3` to find `mavad_main`. This is also where the `.pcap` files will be saved once you run hardware emulation scripts.

### Run your script
* Default nodes mentioned in the repo act as ros nodes, so we need:
  ```bash
  roscore
  ```

* Then in a separate terminal, run:
  ```bash
  script -q /home/$USER/witness.txt
  sudo -i
  cd /home/$USER/hardware/ns-3-allinone/ns-3-dev/cmake-cache/devel/lib/test
  source /opt/ros/melodic/setup.bash
  source /home/$USER/fans_ws/devel/setup.bash
  sudo ip link set wlan0 promisc on
  ./mavad_main
  ```

## Testing on UAV Hardware
* Clone the [control_scripts](https://github.com/Flying-Ad-Hoc-Networks/control_scripts) package into a new workspace and build it.
* The framework is as follows:
  * UAV(X) receives state/mode of UAV(Y) via node_x.cc (mavad_main).
  * node_x.cc publishes received mode to a local rostopic.
  * root_controller in control_scripts subscribes to said topic and finds out what mode previous UAV has changed to.
  * root_controller changes mode of UAV(X) to the received mode until another change in mode is discovered.

## Contributors
* [Subhodeep Choudhury](https://github.com/SnyprDragun)
* [Nitya Ahuja](https://github.com/nitya-ahuja)

## Acknowledgement
This work is supported by the Science and Engineering Research Board (SERB-SRG/2019/001204), Government of India.

## Related Publication
| Field        | Details           |
| ------------- |:-------------:|
| *Title*| Hardware Implementation of FANET Using FANS|
| *Authors*| [Subhodeep Choudhury](https://github.com/SnyprDragun) [Nitya Ahuja](https://github.com/nitya-ahuja) [Abhishek Joshi](https://github.com/abhijoshi2008) [Sarang C. Dhongdi](https://github.com/Sarang-BITS)|
| *Publishing Organization*| ACM|
| *Published in*| International Conference on ns-3 (ICNS3)|
| *Conference / Journal*| Conference|
| *Dates*| 19th August - 21st August 2025|
| *Location*| Ibaraki Osaka Japan|
| *Hosted by*| Ritsumeikan University, Osaka Ibaraki Campus|
| *Co-Hosted by*| Tokushima University Japan|
| *Submitted*| 4th April 2025|
| *Accepted*| 2nd June 2025|
| *Published*| 18th August 2025|
| *DOI*| [10.1145/3747204.3747223](https://dl.acm.org/doi/10.1145/3747204.3747223)|
| *ISBN*| 979-8-4007-1517-4|
| *Location*| On Page(s): 162-167|





