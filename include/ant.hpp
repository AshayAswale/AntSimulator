#pragma once

#include <list>
#include "world.hpp"
#include "config.hpp"
#include "direction.hpp"
#include "number_generator.hpp"
#include "ant_mode.hpp"

#include <iostream>


struct Ant
{
	Ant() = default;

	/**
	 * @brief Construct a new Ant object
	 * 
	 * @param x Colony X position
	 * @param y Colony Y position
	 * @param angle Starting angle of the ant (wrt colony)
	 * @param counter_pheromone_arg Will the ants secret counter pheromone?
	 * @param malicious is the current ant malicious?
	 * @param ant_tracing_pattern_arg Should malicious ants trace food pheromone or roam randomly
	 * @param hell_phermn_intensity_multiplier_arg multiplier for the intensity of TO_HELL pheromone
	 */
	Ant(float x, float y, float angle, bool counter_pheromone_arg = false,
		bool malicious=false, AntTracingPattern ant_tracing_pattern_arg = AntTracingPattern::RANDOM, float hell_phermn_intensity_multiplier_arg = 1.0)
		: position(x, y)
		, direction(angle)
		, last_direction_update(RNGf::getUnder(1.0f) * direction_update_period)
		, last_marker(RNGf::getUnder(1.0f) * marker_period)
		, phase(Mode::ToFood)
		, liberty_coef(RNGf::getRange(0.0001f, 0.001f))
		, hits(0)
		, markers_count(0.0f)
	    , is_malicious(malicious)
		, dilusion_counter(DILUSION_MAX)
		, dilusion_patience_threshold(200)
		, counter_thresh(850)
		, ant_tracing_pattern(ant_tracing_pattern_arg)
		, counter_pheromone(counter_pheromone_arg)
		, hell_phermn_intensity_multiplier(hell_phermn_intensity_multiplier_arg)
	{
		static bool mal_ant_counted;
		if(is_malicious)
			if(!mal_ant_counted)
			{
				mal_ant_counted = true;
				first_mal_ant = true;
			}
	}

	void update(const float dt, World& world, bool wreak_havoc)
	{
		updatePosition(world, dt);
		if(is_malicious && wreak_havoc)
		phase = Mode::ToHell;

		if (phase == Mode::ToFood) {
			checkFood(world);
		}

		last_direction_update += dt;
		if (last_direction_update > direction_update_period) {
			findMarker(world, dt);
			direction += RNGf::getFullRange(direction_noise_range);
			last_direction_update = 0.0f;
		}

		last_marker += dt;
		if (last_marker >= marker_period) {
			addMarker(world);
		}

		direction.update(dt);	
	}

	void updatePosition(World& world, float dt)
	{
		sf::Vector2f v = direction.getVec();
		const sf::Vector2f next_position = position + (dt * move_speed) * v;
		const HitPoint intersection = world.markers.getFirstHit(position, v, dt * move_speed);
		if (intersection.cell) {
			++hits;
			v.x *= intersection.normal.x ? -1.0f : 1.0f;
			v.y *= intersection.normal.y ? -1.0f : 1.0f;
			direction.setDirectionNow(v);
			const uint32_t hits_threshold = 8;
			if (hits > hits_threshold) {
				// If an ant gets stuck, reset its position
				position = Conf::COLONY_POSITION;
			}
		}
		else {
			hits = 0;
			position += (dt * move_speed) * v;
			// Ants outside the map go back to home
			position.x = (position.x < 0.0f || position.x > Conf::WIN_WIDTH) ? Conf::COLONY_POSITION.x : position.x;
			position.y = (position.y < 0.0f || position.y > Conf::WIN_HEIGHT) ? Conf::COLONY_POSITION.y : position.y;
		}
	}

	void checkFood(World& world)
	{
		if (world.markers.isOnFood(position)) {
			phase = Mode::ToHome;
			direction.addNow(PI);
			world.markers.pickFood(position);
			// if(!is_malicious) 
				markers_count = 0.0f;
			dilusion_counter = DILUSION_MAX;
			food_bits_taken_counter++;
			found_food = true;
			return;
		}
	}

	static int getFoodBitsTaken()
	{
		return food_bits_taken_counter;
	}

	static int getFoodBitsDelivered()
	{
		return food_bits_delivered_counter;
	}

	static void resetFoodBitsCounters()
	{
		food_bits_taken_counter = 0;
		food_bits_delivered_counter = 0;
	}

	static void setDilusionMax(float max_value)
	{
		DILUSION_MAX = max_value;
	}

	static void setDilusionIncrement(float increment_value)
	{
		DILUSION_INCREMENT = increment_value;
	}

	void checkColony(const sf::Vector2f colony_position)
	{
		if (getLength(position - colony_position) < colony_size) {
			if (phase == Mode::ToHome) {
				phase = Mode::ToFood;
				direction.addNow(PI);
				food_bits_delivered_counter++;
				delivered_food_home = true;
			}
			// if(!is_malicious)
				markers_count = 0.0f;
		}
	}
	bool nearColony(const sf::Vector2f colony_position, float atol = 15.0f){
		if (getLength(position - colony_position) < colony_size + atol) {
			if (phase == Mode::ToFood) {
				return true;
			}	
		}
		return false;
	}
	void findMarker(World& world, float dt)
	{
		// Init
		const float sample_angle_range = PI * 0.8f;
		const float current_angle = direction.getCurrentAngle();
		float max_intensity = 0.0f;
		sf::Vector2f max_direction;
		WorldCell* max_cell = nullptr;
		// Sample the world
		const uint32_t sample_count = 32;
		bool on_food_path = false;
		for (uint32_t i(sample_count); i--;) {
			// Get random point in range
			const float sample_angle = current_angle + RNGf::getRange(sample_angle_range);
			const float distance = RNGf::getUnder(marker_detection_max_dist);
			const sf::Vector2f to_marker(cos(sample_angle), sin(sample_angle));
			auto* cell = world.markers.getSafe(position + distance * to_marker);
			// Check cell
			if (!cell) {
				continue;
			}
			// Check for food or colony
			if (cell->permanent[static_cast<uint32_t>(phase)]) {
				max_direction = to_marker;
				break;
			}
			// Check for the most intense marker
			float intensity;
			if(phase == Mode::ToHell)
			{
				float value_1, value_2;
				if(ant_tracing_pattern == AntTracingPattern::RANDOM)
				{
					value_1 = 0;
					value_2 = 0;
				}
				else if(ant_tracing_pattern == AntTracingPattern::FOOD)
				{
					value_1 = cell->intensity[static_cast<uint32_t>(Mode::ToFood)];
					value_2 = cell->intensity[static_cast<uint32_t>(Mode::ToHell)];
				}
				else
				{
					value_1 = cell->intensity[static_cast<uint32_t>(Mode::ToHome)];
					value_2 = 0;//cell->intensity[static_cast<uint32_t>(Mode::ToHell)];
				}
				intensity = std::max(value_1, value_2);
			}
			else if(phase == Mode::ToFood)
			{
				float temp_intensity = 0;
				float value_1 = cell->intensity[static_cast<uint32_t>(Mode::ToFood)];
				float value_2 = cell->intensity[static_cast<uint32_t>(Mode::ToHell)];
				float ctr_phrmn_intns = cell->intensity[static_cast<uint32_t>(Mode::CounterPhr)];
				temp_intensity = std::max(value_1, value_2);
				// intensity = temp_intensity;
				
				if (ctr_phrmn_intns < temp_intensity)
					intensity = temp_intensity;
				else
					intensity = 0;
			}
			else
				intensity = cell->intensity[static_cast<uint32_t>(phase)];
			
			if (intensity > max_intensity) {
				max_intensity = intensity;
				max_direction = to_marker;
				max_cell = cell;

			}
			// Randomly choose own path
			if (RNGf::proba(liberty_coef)) {
				break;
			}
		}
		// Update direction
		
		if (max_intensity) {
			if (RNGf::proba(0.4f) && (phase == Mode::ToFood)) {
				max_cell->intensity[static_cast<uint32_t>(phase)] *= 0.99f;
				if (counter_pheromone)
				{
					// std::cout<<"Okay";
					const float coef = 0.01f;
					dilusion_counter = dilusion_counter > 0 ? dilusion_counter - 1.0f : 0;
					const float intensity = 1000.0f * exp(-coef * (dilusion_counter));
					world.addMarker(position, Mode::CounterPhr, intensity);
					// std::cout<<intensity<<" ";
				}
			}
			direction = getAngle(max_direction);
		}
		else
			// dilusion_counter = DILUSION_MAX;
			dilusion_counter = std::min(DILUSION_MAX, dilusion_counter+DILUSION_INCREMENT);
	}

	void addMarker(World& world)
	{
		markers_count += marker_period;
		const float coef = 0.01f;
		float intensity = 1000.0f * exp(-coef * markers_count);
		Mode trace;
		if(phase == Mode::ToHell)
		{
			trace = Mode::ToHell;
			intensity *= hell_phermn_intensity_multiplier;
			// if(first_mal_ant)
			// 	std::cout<<markers_count<<"  ";
			// intensity = 1000.0f * hell_phermn_intensity_multiplier;
			// std::cout<<intensity<<"  ";
		}
		else 
			trace = phase == Mode::ToFood ? Mode::ToHome : Mode::ToFood;
		world.addMarker(position, trace, intensity);
		// else
		//   world.addMarker(position, Mode::ToFood, intensity);
		last_marker = 0.0f;
	}

	void render_food(sf::RenderTarget& target, const sf::RenderStates& states) const
	{
		if (phase == Mode::ToHome) {
			const float radius = 2.0f;
			sf::CircleShape circle(radius);
			circle.setOrigin(radius, radius);
			circle.setPosition(position + length * 0.65f * direction.getVec());
			circle.setFillColor(Conf::FOOD_COLOR);
			target.draw(circle, states);
		}
	}

	void render_in(sf::VertexArray& va, const uint32_t index) const
	{
		const sf::Vector2f dir_vec(direction.getVec());
		const sf::Vector2f nrm_vec(-dir_vec.y, dir_vec.x);

		va[index + 0].position = position - width * nrm_vec + length * dir_vec;
		va[index + 1].position = position + width * nrm_vec + length * dir_vec;
		va[index + 2].position = position + width * nrm_vec - length * dir_vec;
		va[index + 3].position = position - width * nrm_vec - length * dir_vec;
	}

	bool didAntFindFood()
	{
		return found_food;
	}

	bool didAntDeliverFood()
	{
		return delivered_food_home;
	}

	// Parameters
	const float width = 3.0f;
	const float length = 4.7f;
	const float move_speed = 50.0f;
	const float marker_detection_max_dist = 40.0f;
	const float direction_update_period = 0.125f;
	const float marker_period = 0.125f;
	const float direction_noise_range = PI * 0.1f;
	const float colony_size = 20.0f;

	Mode phase;
	sf::Vector2f position;
	Direction direction;
	uint32_t hits;

	float last_direction_update;
	float markers_count;
	float last_marker;
	float liberty_coef;
	float dilusion_counter;
	inline static float DILUSION_MAX;
	float dilusion_patience_threshold;
	float markers_count_dilusion;
	float counter_thresh;
	bool is_malicious;
	bool malicious_ants_focus;
	AntTracingPattern ant_tracing_pattern;
	bool counter_pheromone;
	float hell_phermn_intensity_multiplier;
	inline static int food_bits_taken_counter;
	inline static int food_bits_delivered_counter;
	inline static float DILUSION_INCREMENT;
	bool found_food = false;
	bool delivered_food_home = false;
	bool first_mal_ant = false;
};
