#include <SFML/Graphics.hpp>
#include <vector>
#include <list>
#include <fstream>
#include "colony.hpp"
#include "config.hpp"
#include "display_manager.hpp"
#include "tinyxml2.h"

#include <stdio.h> // for sprintf()

#include <iostream> // for console output
#include <string>	// for std::string
#include <ctime>
#include <sstream>

/****************************************************************************************
************************ CHANGE THESE PARAMETERS FOR TRIALS ************************
****************************************************************************************/
/*
 * @param sim_config.gui_display:: Do you want GUI? If set to true, you can see the simulation
 * @param sim_config.gui_fullscreen:: Do you want GUI to be fullscreen? Useful to turn off since some display configuration may crash at fullscreen
 * @param sim_config.sim_steps:: Number of steps of simulation (Will not be in effect for GUI)
 * @param sim_config.sim_iterations:: Run the same configured iteration these number of times
 * @param sim_config.total_ant_number:: Total number of ants in the simulation
 * @param sim_config.malicious_fraction:: Probability of an ant being malicious (fraction of ants being malicious)
 * @param sim_config.malicious_timer_wait:: Delay after which the attack is launched
 * @param sim_config.malicious_focus::  Should the attack be focused towards food
 * @param sim_config.malicious_tracing_pattern::   Should malicious ants trace food pheromone or roam randomly
 * @param sim_config.patience_activation:: Will the ants secrete counter pheromone?
 * @param sim_config.patience_refill_period_vec:: Period(s) needed for counter pheromone to return to the max value
 * @param sim_config.patience_evaporation_mult:: Multiplier for the evaporation rate of the fake food pheromone
 * @param sim_config.patience_max_val_vec::	What is(are) the maximum value(s) for the counter pheromone?
 * @param sim_config.malicious_intensity_mult:: Multiplier for the intensity of the fake food pheromone
 * @param sim_config.malicious_evaporation_mult:: Multiplier for the evaporation of the fake food pheromone
 */
struct SimulationConfiguration
{
	SimulationConfiguration(){};

	void ParsePatienceRefillPeriodVec(const std::string &str_arr)
	{
		std::istringstream ss(str_arr);
		float val;
		while (ss >> val)
		{
			patience_refill_period_vec.push_back(val);
		}

		patience_refill_period_itr = patience_refill_period_vec.begin();
	};

	void ParseMaxPatienceVec(const std::string &str_arr)
	{
		std::istringstream ss(str_arr);
		float val;
		while (ss >> val)
		{
			patience_max_val_vec.push_back(val);
		}

		patience_max_val_itr = patience_max_val_vec.begin();
	};

	void ParseTracingPattern(const std::string &str)
	{
		if (str == "FOOD")
		{
			malicious_tracing_pattern = AntTracingPattern::FOOD;
		}
		else if (str == "RANDOM")
		{
			malicious_tracing_pattern = AntTracingPattern::RANDOM;
		}
		else if (str == "HOME")
		{
			malicious_tracing_pattern = AntTracingPattern::HOME;
		}
		else
		{
			throw std::invalid_argument("Invalid tracing pattern!");
		}
	};

	bool gui_display = false;

	bool gui_fullscreen = false;

	int sim_steps = 50000;

	int sim_iterations = 100;

	int total_ant_number = 1024;

	bool patience_activation = false;

	std::vector<float> patience_max_val_vec;

	std::vector<float> patience_refill_period_vec;

	std::vector<float>::const_iterator patience_max_val_itr;

	std::vector<float>::const_iterator patience_refill_period_itr;

	float patience_evaporation_mult = 1.0;

	bool malicious_focus = false;

	int malicious_timer_wait = 100;

	float malicious_fraction = 1e-2;

	float malicious_intensity_mult = 1;

	float malicious_evaporation_mult = 10.0;

	AntTracingPattern malicious_tracing_pattern;

	std::string csv_prefix;

	std::string food_map_path;
};

SimulationConfiguration sim_config; // define as a global variable

std::string getExperimentSpecificName(int iteration)
{
	std::string DISPLAY_GUI_string = "_DISPLAY_GUI-" + std::to_string(sim_config.gui_display);
	std::string SIMULATION_STEPS_string = "_SIM_STEPS-" + std::to_string(sim_config.sim_steps);
	std::string SIMULATION_ITERATIONS_string = "_SIM_ITERS-" + std::to_string(sim_config.sim_iterations);
	std::string malicious_fraction_string = "_mal_frac-" + std::to_string(sim_config.malicious_fraction);
	std::string malicious_timer_wait_string = "_mal_delay-" + std::to_string(sim_config.malicious_timer_wait);
	std::string malicious_ants_focus_string = "_mal_ants_focus-" + std::to_string(sim_config.malicious_focus);
	std::string ant_tracing_pattern_string = "_ant_tracing-" + std::to_string(sim_config.malicious_tracing_pattern);
	std::string counter_pheromone_string = "_ctr_pherm-" + std::to_string(sim_config.patience_activation);
	std::string hell_phermn_intensity_multiplier_string = "_hell_phermn_intens-" + std::to_string(sim_config.malicious_intensity_mult);
	std::string hell_phermn_evpr_multi_string = "_hell_phermn_evpr-" + std::to_string(sim_config.malicious_evaporation_mult);
	std::string dilusion_max_string = "_dil_max-" + std::to_string(*sim_config.patience_max_val_itr);
	std::string dilusion_increment_string = "_dil_incr-" + std::to_string(*sim_config.patience_refill_period_itr);
	std::string iteration_string = "_iter-" + std::to_string(iteration);

	return SIMULATION_STEPS_string + SIMULATION_ITERATIONS_string + malicious_fraction_string + malicious_timer_wait_string + malicious_ants_focus_string + ant_tracing_pattern_string + counter_pheromone_string + hell_phermn_intensity_multiplier_string + hell_phermn_evpr_multi_string + dilusion_max_string + dilusion_increment_string + iteration_string;
}

void loadUserConf()
{
	tinyxml2::XMLDocument doc;

	// Populate simulation parameters only if file exists
	try
	{
		if (doc.LoadFile("config.xml") != 0)
		{
			throw std::ios_base::failure("No \"config.xml\" file found!");
		}

		const char *temp_str;
		tinyxml2::XMLElement *root = doc.FirstChildElement("antsim");

		// Get GUI settings
		tinyxml2::XMLElement *gui_element = root->FirstChildElement("gui");
		sim_config.gui_display = gui_element->FirstChildElement("activate")->BoolAttribute("bool");
		sim_config.gui_fullscreen = gui_element->FirstChildElement("fullscreen")->BoolAttribute("bool");

		// Get simulation settings
		tinyxml2::XMLElement *sim_element = root->FirstChildElement("simulation");
		sim_element->FirstChildElement("map")->QueryStringAttribute("path", &temp_str);
		sim_config.food_map_path = std::string(temp_str);
		sim_config.sim_steps = sim_element->FirstChildElement("steps")->IntAttribute("int");
		sim_config.sim_iterations = sim_element->FirstChildElement("iterations")->IntAttribute("int");

		// Get total ant settings
		tinyxml2::XMLElement *total_ants_element = root->FirstChildElement("total_ants");
		sim_config.total_ant_number = total_ants_element->FirstChildElement("number")->IntAttribute("int");
		Conf::ANTS_COUNT = sim_config.total_ant_number; // @todo: this shouldn't really be done this way, but the config file has been hardcoded

		// Get patience/cautionary settings
		tinyxml2::XMLElement *patience_element = root->FirstChildElement("patience");
		sim_config.patience_activation = patience_element->FirstChildElement("activate")->BoolAttribute("bool");
		patience_element->FirstChildElement("refill_period_range")->QueryStringAttribute("str_arr", &temp_str);
		sim_config.ParsePatienceRefillPeriodVec(std::string(temp_str));
		patience_element->FirstChildElement("max_range")->QueryStringAttribute("str_arr", &temp_str);
		sim_config.ParseMaxPatienceVec(std::string(temp_str));
		sim_config.patience_evaporation_mult = patience_element->FirstChildElement("pheromone_evaporation_multiplier")->FloatAttribute("float");

		// Get malicious ants settings
		tinyxml2::XMLElement *malicious_element = root->FirstChildElement("malicious_ants");
		sim_config.malicious_fraction = malicious_element->FirstChildElement("fraction")->FloatAttribute("float");
		sim_config.malicious_focus = malicious_element->FirstChildElement("focus")->BoolAttribute("bool");
		sim_config.malicious_timer_wait = malicious_element->FirstChildElement("timer")->IntAttribute("int");
		sim_config.malicious_intensity_mult = malicious_element->FirstChildElement("pheromone_intensity_multiplier")->FloatAttribute("float");
		sim_config.malicious_evaporation_mult = malicious_element->FirstChildElement("pheromone_evaporation_multiplier")->FloatAttribute("float");
		malicious_element->FirstChildElement("tracing_pattern")->QueryStringAttribute("type", &temp_str);
		sim_config.ParseTracingPattern(std::string(temp_str));

		// Get CSV filepath
		root->FirstChildElement("csv_output")->QueryStringAttribute("prefix", &temp_str);

		sim_config.csv_prefix = std::string(temp_str);
	}
	catch (const std::exception &e)
	{
		std::cout << e.what() << std::endl;
		exit(1);
	}
}

void setStaticVariables()
{
	WorldCell::setHellPhermnEvprMulti(sim_config.malicious_evaporation_mult);
	Ant::resetFoodBitsCounters();
	Ant::setDilusionMax(*sim_config.patience_max_val_itr);
	Ant::setDilusionIncrement((*sim_config.patience_max_val_itr) / (*sim_config.patience_refill_period_itr));
}

void initWorld(World &world, Colony &colony)
{
	setStaticVariables();
	for (uint32_t i(0); i < 64; ++i)
	{
		float angle = float(i) / 64.0f * (2.0f * PI);
		world.addMarker(colony.position + 16.0f * sf::Vector2f(cos(angle), sin(angle)), Mode::ToHome, 10.0f, true);
	}

	sf::Image food_map;
	if (food_map.loadFromFile(sim_config.food_map_path))
	{
		for (uint32_t x(0); x < food_map.getSize().x; ++x)
		{
			for (uint32_t y(0); y < food_map.getSize().y; ++y)
			{
				const sf::Vector2f position = float(world.markers.cell_size) * sf::Vector2f(to<float>(x), to<float>(y));
				if (food_map.getPixel(x, y).g > 100)
				{
					///////////////////
					// FOOD POSITION
					world.addFoodAt(position.x / 10, position.y / 10, 5);
				}
				else if (food_map.getPixel(x, y).r > 100)
				{
					world.addWall(position);
				}
			}
		}
	}
}

void updateColony(World &world, Colony &colony)
{
	const static float dt = 0.016f;
	colony.update(dt, world);
	world.update(dt);
}

void oneExperiment(int i)
{
	std::ofstream myfile;
	const static float dt = 0.016f;
	const static int datapoints_to_record = 100;
	static int skip_steps = sim_config.sim_steps / datapoints_to_record;
	static std::string file_name_prefix = sim_config.csv_prefix;
	static int x = 0;

	std::string filepath = file_name_prefix + getExperimentSpecificName(i) + ".csv";
	try
	{
		myfile.open(filepath);
		if (!myfile.is_open())
		{
			throw std::ios_base::failure("Cannot create path to " + filepath);
		} // ensure the file can be created
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		exit(1);
	}

	float food_found_per_ant = 0.0;
	float food_delivered_per_ant = 0.0;
	float fraction_of_ants_found_food = 0.0;
	float fraction_of_ants_delivered_food = 0.0;

	setStaticVariables();
	World world(Conf::WORLD_WIDTH, Conf::WORLD_HEIGHT);
	Colony colony(Conf::COLONY_POSITION.x,
				  Conf::COLONY_POSITION.y, Conf::ANTS_COUNT,
				  sim_config.malicious_fraction,
				  sim_config.malicious_timer_wait,
				  sim_config.malicious_focus,
				  sim_config.malicious_tracing_pattern,
				  sim_config.patience_activation,
				  sim_config.malicious_intensity_mult);
	initWorld(world, colony);

	for (int j = 0; j < sim_config.sim_steps; j++)
	{
		updateColony(world, colony);
		if (j % skip_steps == 0)
		{
			food_found_per_ant = float(Ant::getFoodBitsTaken()) / float(sim_config.total_ant_number);		  // Total  number of Ants
			food_delivered_per_ant = float(Ant::getFoodBitsDelivered()) / float(sim_config.total_ant_number); // Total  number of Ants
			fraction_of_ants_found_food = float(Colony::getAntsThatFoundFood()) / float(sim_config.total_ant_number);
			fraction_of_ants_delivered_food = float(Colony::getAntsThatDeliveredFood()) / float(sim_config.total_ant_number);
			myfile << (food_found_per_ant) << "," << food_delivered_per_ant << ","
				   << fraction_of_ants_found_food << "," << fraction_of_ants_delivered_food << std::endl;
		}
	}
	myfile.close();
	std::cout << "Experiment " << x++ << " Done" << std::endl;
}

void simulateAnts()
{
	/**
	 * @brief This loop will start a new colony and run the sim for sim_config.sim_steps number of steps for each trial
	 */
	// Iterate through number of trials/iterations
	for (int i = 0; i < sim_config.sim_iterations; i++)
	{
		/*
			The following nested loops are constructed the way they are because of the original code's
			dependency on global variables. Specifically, the `setStaticVariables()` function uses global
			variables to set the simulation parameters that affect other parts of the code (see `oneExperiment()`).

			Ideally, the dependency on the global variable should be minimized and be explicit (i.e., traceable calls),
			which can be achieved by using it in a single function and passed on to sub-function calls as arguments.
		*/

		// Iterate through all max patience pheromone values
		for (auto &itr = sim_config.patience_max_val_itr; itr != sim_config.patience_max_val_vec.end(); ++itr)
		{
			// Iterate through all patience refill period values
			for (auto &itr = sim_config.patience_refill_period_itr; itr != sim_config.patience_refill_period_vec.end(); ++itr)
			{
				oneExperiment(i); // run single experiment trial
			}

			sim_config.patience_refill_period_itr = sim_config.patience_refill_period_vec.begin();
		}

		sim_config.patience_max_val_itr = sim_config.patience_max_val_vec.begin();
		std::cout << "###########################" << std::endl;
		std::cout << "Iteration " << i << " Done" << std::endl;
		std::cout << "###########################" << std::endl;
	}
	std::cout << "########## DONE ##########" << std::endl;
}

void displaySimulation()
{
	setStaticVariables();
	World world(Conf::WORLD_WIDTH, Conf::WORLD_HEIGHT);
	Colony colony(Conf::COLONY_POSITION.x,
				  Conf::COLONY_POSITION.y,
				  Conf::ANTS_COUNT,
				  sim_config.malicious_fraction,
				  sim_config.malicious_timer_wait,
				  sim_config.malicious_focus,
				  sim_config.malicious_tracing_pattern,
				  sim_config.patience_activation,
				  sim_config.malicious_intensity_mult);

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;
	initWorld(world, colony);
	auto sf_gui_display_style = sim_config.gui_fullscreen ? sf::Style::Fullscreen : sf::Style::Default;
	sf::RenderWindow window(sf::VideoMode(Conf::WIN_WIDTH, Conf::WIN_HEIGHT), "AntSim", sf_gui_display_style, settings);
	window.setFramerateLimit(60);

	DisplayManager display_manager(window, window, world, colony);

	sf::Vector2f last_clic;
	int c = 0;
	int C = -100;

	while (window.isOpen())
	{
		display_manager.processEvents();
		// Add food on clic
		if (display_manager.clic)
		{
			const sf::Vector2i mouse_position = sf::Mouse::getPosition(window);
			const sf::Vector2f world_position = display_manager.displayCoordToWorldCoord(sf::Vector2f(to<float>(mouse_position.x), to<float>(mouse_position.y)));
			const float clic_min_dist = 2.0f;
			if (getLength(world_position - last_clic) > clic_min_dist)
			{
				if (display_manager.wall_mode)
				{
					world.addWall(world_position);
				}
				else if (display_manager.remove_wall)
				{
					world.removeWall(world_position);
				}
				else
				{
					world.addFoodAt(world_position.x, world_position.y, 20);
				}
				last_clic = world_position;
			}
		}

		if (!display_manager.pause)
		{
			updateColony(world, colony);
			// std::cout<<std::endl;
		}

		if (c++ > C)
		{
			c = 0;
			window.clear(sf::Color(94, 87, 87));

			display_manager.draw();

			window.display();
		}
	}
}

int main()
{
	Conf::loadTextures();

	loadUserConf();
	if (sim_config.gui_display)
		displaySimulation();
	else
		simulateAnts();

	// Free textures
	Conf::freeTextures();

	return 0;
}

#if defined(_WIN32)
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline,
					 int cmdshow)
{
	return main();
}
#endif