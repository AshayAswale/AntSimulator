# AntSimulator

Very simple ants simulator.

# Installation

## Prerequisites

In order to compile this project you will need to:
 - have [SFML](https://www.sfml-dev.org/index.php) installed on your system. If you don't know how to do it see [this link](https://www.sfml-dev.org/tutorials/2.5/#getting-started).
 - have [CMake](https://cmake.org/) installed

Please note that you might need reasonably modern computer in order to run this simulator at the intended framerates.


## Compilation

Detailed explanation [here](https://preshing.com/20170511/how-to-build-a-cmake-based-project/)

### On Linux with `install.sh`
- Go in the repo folder

`cd the/repo/location`

- Execute `install.sh` script

`./install.sh`

### On Windows with CMake GUI and Visual Studio
 - Install the right SFML version or compile it (see [this](https://www.sfml-dev.org/tutorials/2.5/start-vc.php))
 - Run CMake
 - Select the repo location
 
![Cmake 1](https://github.com/johnBuffer/AntSimulator/blob/master/img/cmake_1.PNG)
 - Click on `Configure`, if you have installed the `x64` version of SFML, in the pop up window select `x64` in the `Optionnal platform for generator` drop down

![Cmake 2](https://github.com/johnBuffer/AntSimulator/blob/master/img/cmake_2.PNG)
 - Click on `Finish`
 - Click on `Generate`

![Cmake 3](https://github.com/johnBuffer/AntSimulator/blob/master/img/cmake_3.PNG)
 - You can now open the generated project and build it.

# How to run
After building the project, populate the configuration file `config.xml` as follows:
```xml
<?xml version="1.1" encoding="UTF-8"?>
<antsim>
    <gui>
        <!-- GUI activation -->
        <!-- Options: "true", "false" -->
        <activate bool="false" /> <!-- activate GUI -->

        <!-- Fullscreen mode activation; only relevant if GUI is activated -->
        <!-- Options: "true", "false" -->
        <fullscreen bool="false" />
    </gui>
    <simulation>
        <!-- Path to the food map image, relative to the working directory (where you run the simulator from) -->
        <map path="/home/kchin/hacking_the_colony/map.bmp" />

        <!-- Number of simulation steps (1 * dt per simulation step) -->
        <steps int="50000" />

        <!-- Number of trials to repeat -->
        <iterations int="20" />
    </simulation>
    <total_ants>
        <!-- Number of total ants (malicious and not) to simulate -->
        <number int="1024" />
    </total_ants>
    <patience>
        <!-- Counter/patience/cautionary pheromone activation by the non-malicious ants -->
        <!-- Options: "true", "false" -->
        <activate bool="true" />

        <!-- Period for the patience pheromone to fill up from 0 to max (without any interruption) in units of update cycles (8 * dt per update cycle) -->
        <!-- Options: a single floating point value or a list/array of space delimited floating point values -->
        <!-- E.g., str_arr="100", str_arr="1.0 5 10 20 25.0", etc. -->
        <refill_period_range str_arr="1000" />

        <!-- Maximum value of cautionary pheromone -->
        <!-- Options: a single floating point value or a list/array of space delimited floating point values -->
        <!-- E.g., str_arr="15.0", str_arr="5 10 12.5 20 25.0", etc. -->
        <max_range str_arr="1000" />

        <!-- Evaporation rate of patience pheromone in units of simulation steps (1 * dt) -->
        <pheromone_evaporation_multiplier float="1" />
    </patience>
    <malicious_ants>
        <!-- Fraction of malicious ants out of the total number of ants -->
        <!-- Options: a floating point value between (inclusive) 0 to 1 -->
        <fraction float="0.125" />

        <!-- Directional focus (by malicious ants, towards the food source) activation -->
        <!-- Options: "true", "false" -->
        <focus bool="false" />

        <!-- Minimum number of simulation steps of headstart before malicious ants begin their attack -->
        <timer int="100" />

        <!-- Intensity of fake food pheromone -->
        <pheromone_intensity_multiplier float="1" />

        <!-- Evaporation rate of fake food pheromone in units of simulation steps (1 * dt) -->
        <pheromone_evaporation_multiplier float="5" />

        <!-- Tracing pattern of malicious ants: do the ants follow food, home, or neither pheromones? -->
        <!-- Options: "FOOD", "HOME", "RANDOM" -->
        <tracing_pattern type="FOOD" />
    </malicious_ants>

    <!-- Output CSV data file prefix -->
    <csv_output prefix="output_folder/output_prefix" />
</antsim>
```

Once the configuration file is prepared, run the simulator in the same directory where `config.xml` resides. (If your installation in the above steps was successful, you should have the executable in the `build` directory of this repository.)
```
$ build/AntSimulator
```
The simulator will now start and the output data will be available in the location you specify in `csv_output`.

In the CSV files, there will be four columns. They describe
* column 1: the average food bit collected by **all** the ants.
* column 2: the average food bit delivered by **all** the ants.
* column 3: the fraction of cooperator (non-malicious) ants that collected food.
* column 4: the fraction of cooperator (non-malicious) ants that delivered food.

# Commands

|Command|Action|
|---|---|
|**P**|Pause/Unpause the simulation|
|**M**|Toggle markers drawing|
|**A**|Toggle ants drawing|
|**S**|Toggle max speed mode|
|**W**|Toggle Wall mode|
|**E**|Toggle Wall erase mode|
|**Right click**|Add food|
|**Left click**|Move view|
|**Wheel**|Zoom|
