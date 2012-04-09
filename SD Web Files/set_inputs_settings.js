function set_inputs(selector,array){			//this takes a jQuery selector (an input or select) and sets it w/ values from an array
	j = 0;										//init a counter var
	$(selector).each(function(){				//loop through each <select>
		for (i in array){						//loop through each element in array
			if (j == i){						//if this element # corresponds w/ this <select>
				$(this).val(array[i]);			//set the value
				
				//if the selector is a radio (as on the settings page)
				if ($(selector).attr("type") == "radio" && array[i] == 1){	//if radio and array value = 1
					$(this).attr("checked", "checked"); //Check it (the browser unchecks the other)
				};
				
				//if the selector is the output duration, we have to do some fancy stuff about the hidden fields
				if (selector == "input.output_duration") {
					temp_selected = $(this).parent();							//reduce jquery calls
					if (array[i] > 0) {											//if the duration is not zero
						temp_selected.show();									//show the input field
						temp_selected.prev().val("for");						//choose the "for" select
						temp_selected.prev().children().last().html("for");		//and change the "for..." to "for"
					}else{													//if it is 0
						temp_selected.hide();									//hide the field
						temp_selected.prev().val("constantly");					//and choose "constantly" to complete the sentence
					}
				};
			};
		};
		j ++;									//increment counter
	});
};//end set_inputs()