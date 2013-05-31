// Sound Module pseudocode structure

void setup(){
  //variables
	ambient = false;
	current_trigger_sound = 0;
	random_mode = false;

	//SD card logic
		//setup SD card...
	if (SD card exists && filesystem)
		proceed
	}else{
		flash lights or play error sound?
	}

	//sound file logic
	if (ambient.wav exists)
		ambient = true;
	if (trig*.wav exists)
		trigger_sounds[] = count of trig1.wav ... trign.wav
	if (file random.txt exists)
		random_mode = true;
}

void loop(){
	if (ambient) {
		play ambient.wave in loop
	}
	if (triggered) {
		if (random_mode){													// if in random_mode play random sound
			play trigger_sounds[random number <= trigger_sounds.length];
		}else{																//if not in random_mode, play next sound in sequence
			play trigger_sounds[current_trigger_sound];

			if (current_trigger_sound > trigger_sounds.length){
				current_trigger_sound = 0;									// back to the beginning
			}else{
				current_trigger_sound ++;
			}
		}
	}
}
