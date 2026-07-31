// 15 agents available in the simulation per world
const FOREST_AGENTS = [
  { id: 0, name: "Squirrel 1" },
  { id: 1, name: "Squirrel 2" },
  { id: 2, name: "Squirrel 3" },
  { id: 3, name: "Crow" },
  { id: 4, name: "Chiffchaff" },
  { id: 5, name: "Woodpecker" },
  { id: 6, name: "Pheasant" },
  { id: 7, name: "Deer 1" },
  { id: 8, name: "Deer 2" },
  { id: 9, name: "Fox" },
  { id: 10, name: "Old Oak Branch" },
  { id: 11, name: "Woodmouse" },
  { id: 12, name: "Badger" },
  { id: 13, name: "Owl" },
  { id: 14, name: "Wind Gust" }
];

const CITY_AGENTS = [
  { id: 0, name: "Shopkeeper" },
  { id: 1, name: "Commuter 1" },
  { id: 2, name: "Commuter 2" },
  { id: 3, name: "Accordion Busker" },
  { id: 4, name: "Cafe Customer 1" },
  { id: 5, name: "Cafe Customer 2" },
  { id: 6, name: "Delivery Driver" },
  { id: 7, name: "Angry Pedestrian" },
  { id: 8, name: "Umbrella Walker" },
  { id: 9, name: "Puddle Splasher 1" },
  { id: 10, name: "Puddle Splasher 2" },
  { id: 11, name: "Cyclist" },
  { id: 12, name: "Car Driver" },
  { id: 13, name: "Tram Operator" },
  { id: 14, name: "Pigeon" }
];

const WATER_AGENTS = [
  { id: 0, name: "Fisherman" },
  { id: 1, name: "Mallard Duck 1" },
  { id: 2, name: "Mallard Duck 2" },
  { id: 3, name: "Mallard Duck 3" },
  { id: 4, name: "Swan" },
  { id: 5, name: "Aggressive Coot 1" },
  { id: 6, name: "Aggressive Coot 2" },
  { id: 7, name: "Fish Shoal" },
  { id: 8, name: "Jumping Fish" },
  { id: 9, name: "Heron" },
  { id: 10, name: "Water Vole" },
  { id: 11, name: "Frog 1" },
  { id: 12, name: "Frog 2" },
  { id: 13, name: "Frog 3" },
  { id: 14, name: "Big Bullfrog" }
];

const STORY_BLOCKS = {
  forest: [
    {
      id: "forest_morning_hunt",
      durationSeconds: 40,
      weather: "clear",
      description: "A squirrel foraging in the morning wakes up the canopy.",
      events: [
        { timeSeconds: 0, agents: [2], description: "Squirrel scurries up a large oak tree", sfx: "forest_woodmice_leaf_scurry-tiny_rapid_footsteps_scattering_dry_leaves-1780056247957.mp3" },
        { timeSeconds: 5, agents: [2], description: "Squirrel begins gnawing on an acorn", sfx: "forest_acorn_bouncing_floor-acorn_bouncing_and_rolling_on_hard_dry_forest_fl-1780056291185.mp3" },
        { timeSeconds: 15, agents: [3], description: "A resting bird gets startled by the noise", sfx: "forest_crow_harsh_caw-crow_harsh_single_cawing_from_high_canopy-1780056254521.mp3" },
        { timeSeconds: 18, agents: [3], description: "The bird takes off in a panic", sfx: "forest_pheasant_takeoff-pheasant_sudden_explosive_noisy_takeoff_from_gro-1780056288926.mp3" },
        { timeSeconds: 25, agents: [2], description: "Squirrel stops, startled, then leaps to another branch", sfx: "forest_squirrel_branch_leap-squirrel_leaping_between_branches_whole_canopy_s-1780056261006.mp3" },
        { timeSeconds: 32, agents: [4], description: "Another bird calls back in the distance", sfx: "forest_chiffchaff_call-chiffchaff_repetitive_two_note_call_from_canopy-1780056293302.mp3" }
      ]
    },
    {
      id: "forest_rainstorm",
      durationSeconds: 60,
      weather: "rain_heavy",
      description: "Heavy rain rolls into the forest, sending animals scattering for cover.",
      events: [
        { timeSeconds: 0, agents: [], description: "First drops of rain hit the canopy", sfx: "forest_rain_forest_beginning-rain_beginning_gently_on_outer_forest_leaves-1780056295330.mp3" },
        { timeSeconds: 8, agents: [7, 8], description: "Deer bark in alarm and run for cover", sfx: "forest_deer_alarm_bark-deer_sharp_explosive_alarm_bark_then_crashing_fl-1780056252394.mp3" },
        { timeSeconds: 15, agents: [], description: "Rain turns into a heavy downpour", sfx: "forest_rain_forest_heavy-heavy_rain_dense_drumming_on_whole_forest_canopy-1780056297302.mp3" },
        { timeSeconds: 30, agents: [2], description: "A squirrel chatters angrily from beneath a branch", sfx: "forest_squirrel_chattering_scold-squirrel_rapid_chattering_scolding_alarm_call-1780056258870.mp3" },
        { timeSeconds: 45, agents: [10], description: "A large branch creaks heavily under the weight of the storm", sfx: "forest_branch_creak_slow-large_branch_creaking_slowly_under_wind_pressure-1780056289145.mp3" }
      ]
    },
    {
      id: "forest_night_forage",
      durationSeconds: 35,
      weather: "foggy",
      description: "Nighttime foragers looking for food in the quiet.",
      events: [
        { timeSeconds: 0, agents: [12], description: "A badger starts rooting around in the leaves", sfx: "forest_badger_rooting_night-badger_rooting_digging_in_leaf_litter_at_night-1780056293140.mp3" },
        { timeSeconds: 10, agents: [12], description: "Badger knocks loose some tree bark", sfx: "forest_bark_stripping_rattle-dead_bark_stripping_falling_from_tree_trunk_piec-1780056299405.mp3" },
        { timeSeconds: 20, agents: [13], description: "An owl hoots inquiringly", sfx: "forest_dawn_chorus_building-forest_dawn_chorus_multiple_birds_building_toget-1780056282769.mp3" }
      ]
    }
  ],
  
  city: [
    {
      id: "city_morning_rush",
      durationSeconds: 45,
      weather: "clear",
      description: "The city wakes up as shopkeepers open and people head to work.",
      events: [
        { timeSeconds: 0, agents: [0], description: "Shopkeeper opens the metal shutter for the day", sfx: "city_shopkeeper_metal_shutter-heavy_metal_shop_shutter_rolling_up_rattling-1780056214815.mp3" },
        { timeSeconds: 10, agents: [1, 5, 6], description: "Commuters walking down the stone pavement", sfx: "city_pedestrian_footsteps_stone-steady_footsteps_walking_on_stone_pavement_echo-1780056221430.mp3" },
        { timeSeconds: 20, agents: [2], description: "A car accelerates away from the intersection", sfx: "city_car_accelerating-car_accelerating_pulling_away_from_junction-1780056199702.mp3" },
        { timeSeconds: 28, agents: [3], description: "A busker starts playing an accordion", sfx: "city_accordion_melody_sweet_and_weathered-1781215108054.mp3" },
        { timeSeconds: 38, agents: [5], description: "Someone throws coins into the busker's case", sfx: "city_busker_coins_case-coins_dropping_into_busker_open_case_echoing-1780056208717.mp3" }
      ]
    },
    {
      id: "city_rain_commute",
      durationSeconds: 50,
      weather: "rain_soft",
      description: "People running through the rain in the afternoon.",
      events: [
        { timeSeconds: 0, agents: [], description: "Rain starts drumming on the shop awnings", sfx: "city_rain_on_awnings-rain_drumming_on_shop_canvas_awnings_overhead-1780056223737.mp3" },
        { timeSeconds: 5, agents: [8], description: "Someone snaps an umbrella open in a hurry", sfx: "city_rain_umbrella_snap-umbrella_snapping_open_in_sudden_downpour-1780056225765.mp3" },
        { timeSeconds: 15, agents: [9, 10], description: "People splash through puddles running for cover", sfx: "city_rain_puddle_splash-footsteps_splashing_through_puddles_on_pavement-1780056226188.mp3" },
        { timeSeconds: 25, agents: [12], description: "A car door slams as someone jumps inside", sfx: "city_car_door_slam-car_door_slamming_shut-1780056200020.mp3" },
        { timeSeconds: 35, agents: [], description: "The street drain starts gurgling with water", sfx: "city_rain_drain_gurgle-rain_drain_gurgling_overflowing_gutter-1780056224086.mp3" }
      ]
    },
    {
      id: "city_afternoon_cafe",
      durationSeconds: 30,
      weather: "clear",
      description: "Afternoon cafe vibes outside.",
      events: [
        { timeSeconds: 0, agents: [2, 3, 4], description: "People talking softly at outdoor cafe", sfx: "city_cafe_ambient_murmur-cafe_indoor_ambient_murmur_laughter_background-1780056206499.mp3" },
        { timeSeconds: 8, agents: [4], description: "Chairs scraping against the stone as someone leaves", sfx: "city_cafe_chairs_scraping-outdoor_cafe_metal_chair_scraping_stone_pavement-1780056204382.mp3" },
        { timeSeconds: 18, agents: [7], description: "An argument breaks out down the street", sfx: "city_argument_raised_voices-sudden_raised_voices_argument_in_street_retreati-1780056219436.mp3" }
      ]
    }
  ],

  water: [
    {
      id: "pond_duck_drama",
      durationSeconds: 40,
      weather: "clear",
      description: "Ducks land on the pond causing a commotion.",
      events: [
        { timeSeconds: 0, agents: [1, 2], description: "Ducks splash down noisily onto the pond", sfx: "water_duck_landing_splash-ducks_landing_noisily_splashing_on_water-1780056164544.mp3" },
        { timeSeconds: 8, agents: [1, 2, 3], description: "Social quacking begins among the group", sfx: "water_duck_social_quacking-small_group_ducks_social_quacking_dabbling-1780056166702.mp3" },
        { timeSeconds: 18, agents: [5, 6], description: "Coots get territorial and start skirmishing", sfx: "water_coot_skirmish_splashing-two_coots_splashing_fighting_wing_beating_on_wat-1780056146639.mp3" },
        { timeSeconds: 30, agents: [1, 2, 3], description: "The ducks take off explosively to escape", sfx: "water_duck_sudden_takeoff-ducks_sudden_noisy_explosive_takeoff_from_water-1780056166657.mp3" }
      ]
    },
    {
      id: "pond_frog_storm",
      durationSeconds: 50,
      weather: "rain_heavy",
      description: "Rain hits the pond, and the frogs enjoy it.",
      events: [
        { timeSeconds: 0, agents: [], description: "Rain begins arriving slowly", sfx: "water_rain_arriving_gradually-rain_beginning_gently_increasing_drumming_on_wat-1780056168780.mp3" },
        { timeSeconds: 10, agents: [], description: "Heavy rain drumming on the water surface", sfx: "water_rain_heavy_on_water-heavy_rain_drumming_hard_on_water_reeds_leaves-1780056168998.mp3" },
        { timeSeconds: 20, agents: [14], description: "A frog plops happily into the water", sfx: "water_frog_single_plop-single_frog_jumping_plopping_into_still_water-1780056155561.mp3" },
        { timeSeconds: 28, agents: [11, 12, 13, 14], description: "The frog chorus begins swelling in the reeds", sfx: "water_frog_chorus_rising-frog_colony_chorus_rising_and_swelling_from_reed-1780056152888.mp3" },
        { timeSeconds: 42, agents: [], description: "A thunderclap causes sudden silence among the frogs", sfx: "water_frog_chorus_sudden_silence-frog_chorus_abruptly_stopping_complete_sudden_si-1780056153384.mp3" }
      ]
    },
    {
      id: "pond_evening_fishing",
      durationSeconds: 35,
      weather: "clear",
      description: "A fisherman sits by the pond at sunset while fish jump.",
      events: [
        { timeSeconds: 0, agents: [8], description: "A single fish jumps and splashes back", sfx: "water_fish_single_jump-single_fish_jumping_splashing_back_into_water-1780056157689.mp3" },
        { timeSeconds: 12, agents: [0], description: "Fisherman rattles their tackle box on the bank", sfx: "water_fisherman_tackle_rattle-fishing_tackle_rattle_rod_click_on_far_bank-1780056179535.mp3" },
        { timeSeconds: 25, agents: [0], description: "Fisherman coughs softly into the evening air", sfx: "water_fisherman_soft_cough-distant_fisherman_quiet_cough_clearing_throat-1780056180451.mp3" }
      ]
    }
  ]
};
