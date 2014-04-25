    sent = "true";
	dx_prev = 0;
	dy_prev = 0;
	console.log("touchscreen is", VirtualJoystick.touchScreenAvailable() ? "available" : "not available");
	var joystick	= new VirtualJoystick({
		container	: document.getElementById('container'),
		mouseSupport	: true,
		stationaryBase	: false,
		baseX		: 100,
		baseY		: 100,				
	});
    n = 0;
	setInterval(function(){
		var dx = joystick.deltaX();
		var dy = joystick.deltaY();
		$('#result').html('<b>Result:</b> '
			+ ' dx:'+dx
			+ ' dy:'+dy
			+ (joystick.right()	? ' right'	: '')
			+ (joystick.up()	? ' up'		: '')
			+ (joystick.left()	? ' left'	: '')
			+ (joystick.down()	? ' down' 	: ''));	
		if(sent == "true" && (dx != dx_prev ||  dy != dy_prev))
		{
			 loadXMLDoc();
		     dx_prev = dx;
		     dy_prev = dy;
			 n = 0;
		}
        else if(dx!=0 || dy!=0)
		{
		     n++;
			 if(n==30){
			  loadXMLDoc();
			  n = 0;
			 }
		}

	},  1000 * 1/30);
					
	var connectFlag = false;
	
	var testdata = {distance_front:-1.000000,
					distance_rear:1.329864,
					ubat_power:12,
					ubat_compute:10,
					v_left:0.000000,
					v_right:-0.000000,
					board_status:[0,0,0,1]}
	
	function loadXMLDoc()
	{
		var msg = 'Values:'
				+ ' dx: '+joystick.deltaX()
				+ ' dy: '+joystick.deltaY();
		
		$.post("whereveryouwant", msg)
			.done(function(data){
				dataHandler(data);
			})
			.fail(function(){
				warningMessage(true);
			})
			.always(function(jqXHR, textStatus){
				console.log(textStatus);
			});

	}
	
	function dataHandler(data){
		notification(parseMessage(data));
	}
	
	function notification(message){
		$("#notification").hide();
		if(message != null && message != ""){
			// Create the message element
			$("#newMsg p").eq(0).html(message);
			$("#notification").show();
			$("#notification").hide(4000);
		}
	}
	
	function warningMessage(isConnected){
		if(isConnected){
			var message ="<div class=\"alert alert-danger alert-dismissable\">" +
			  			"<button type=\"button\" class=\"close\" data-dismiss=\"alert\" aria-hidden=\"true\">&times;</button>" +
			  			"<strong>Oh snap!</strong> Unable to Connect server!" +
						"</div>";
			if(connectFlag){
				$("#warningDialog").dialog("open");
				connectFlag = false;
			}
			$("#warning").html(message);
		}else{
			if(!connectFlag){
				$("#warningDialog").dialog("close");
				connectFlag = true;
			}
			$("#warning").html("");
		}
	}
	
	function parseMessage(data){
		var message = "";
		if(data.distance_front != null && data.distance_front >= 0 & data.distance_front < 1){
			message += "There is an obstacle in the front: " + data.distance_front + " meter.\n";
		}
		if(data.distance_rear != null && data.distance_rear >= 0 & data.distance_rear < 1){
			message += "There is an obstacle in the rear: " + data.distance_rear + " meter.\n";
		}
		if(data.ubat_power != null || data.ubat_compute != null){
			var percentage = Math.min(Math.ceil((Math.max(data.ubat_power, 12) - 10) * 100 / 2), Math.ceil((Math.max(data.ubat_compute, 12) - 10) * 100 / 2));
			batteryDisplay(percentage);
		}
		if(data.v_left != null){
			speedDisplay(Math.ceil(data.v_left * 100));
		}
		if(data.board_status != null && JSON.stringify(data.board_status) != JSON.stringify([1, 1, 1, 1])){
			message += "Nano board error! Error message: " + data.board_status + ".\n";
		}
		return message;
	}
	
	function speedDisplay(percentage){
		$("#speed div div").css("width", percentage + "%");
	}
	
	function batteryDisplay(percentage){
		var battery;
		if(percentage > 80){
			battery = "images/battery-full.png";
		}else if(percentage > 65 && percentage <=80){
			battery = "images/battery-.png";
		}else if(percentage > 20 && percentage <=65){
			battery = "images/battery-half.png";
		}else{
			battery = "images/battery-empty.png";
		}
		if($("#carstatus .panel-heading a img").eq(1).attr("src") != battery){
			$("#carstatus .panel-heading a img").eq(1).attr("src", battery);
		}
	}