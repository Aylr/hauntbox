boolean testget_handler(TinyWebServer& web_server){
	while (client.connected()) {
		if (client.available()) {
			// Read a byte at a time and concatenate it onto the end of the character
			if (client.available() == 0) {
				// Split readString into columns with comma-separated values.  Columns
				// Now turn the array of strings into arrays of numbers.  Each array is
				// Now print out all the values to make sure it all worked
			}
		}//client.available()
		Serial.println("end client.available()");
	}//while client.connected()
	Serial.println("end client.connected()");   
	return true; //exit the handler 
}
