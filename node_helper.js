'use strict';
const NodeHelper = require('node_helper');
const { spawn, exec } = require('child_process');
var readline = require('readline') 
var started = false
var cAppStarted = false
//var websockets = require("websockets");

module.exports = NodeHelper.create({

	sub_prozess_start: function () {
		const self = this;
		self.imagehandler_cpp = spawn('modules/' + this.name + '/image_handler/build/image_handler',[self.config.image_width, self.config.image_height, self.config.rotation, "modules/" + self.name + "/image_handler/icons/" , self.config.debug]);
		self.imagehandler_cpp.stdout.on('data', (data) => {

			var data_chunks = `${data}`.split('\n');
			
			data_chunks.forEach( chunk => {

			if (chunk.length > 0) {
			try{
				var parsed_message = JSON.parse(chunk)
				if (parsed_message.hasOwnProperty('IMAGE_HANDLER_FPS')){
					self.sendSocketNotification('IMAGE_HANDLER_FPS', parsed_message.IMAGE_HANDLER_FPS);
					//console.log("[" + self.name + "] " + JSON.stringify(parsed_message));
				}else if (parsed_message.hasOwnProperty('STATUS')){
					console.log("[" + self.name + "] status received: " + JSON.stringify(parsed_message));
				}else if (parsed_message.hasOwnProperty('INIT')){
					console.log("[" + self.name + "] status received: " + JSON.stringify(parsed_message));
					if(parsed_message.INIT === 'DONE'){
						self.sendSocketNotification('INIT', parsed_message.INIT);
					}
				}
			}
			catch(err) {	
			console.log(err)
			}
  			//console.log(chunk);
			}
			});
			});
	},


	//var data = {"FPS": payload}
	//self.objectDet.stdin.write(payload.toString() + "\n");

	// Subclass socketNotificationReceived received.
	socketNotificationReceived: function(notification, payload) {
	const self = this;
	try {	
		// console.log("[" + self.name + "] got socket notification: " + notification + ": " + payload);
		if(notification === 'CONFIG') {
			this.config = payload
			console.log("[" + self.name + "] Starting with config: " + this.config);
			this.sub_prozess_start(); 
			started = true;
    		}else if(notification === 'CENTER_DISPLAY'){
			var data = {"SET": payload};
			self.imagehandler_cpp.stdin.write(JSON.stringify(data) + "\n");
		}else if (notification === 'DETECTED_GESTURES'){
			self.imagehandler_cpp.stdin.write(JSON.stringify(payload) + "\n");
		}else if (notification === 'DETECTED_OBJECTS'){
			self.imagehandler_cpp.stdin.write(JSON.stringify(payload) + "\n");
		}else if (notification === 'DETECTED_FACES'){
			self.imagehandler_cpp.stdin.write(JSON.stringify(payload) + "\n");
		}else if (notification === 'RECOGNIZED_PERSONS'){
			self.imagehandler_cpp.stdin.write(JSON.stringify(payload) + "\n");
	};
	} catch (err) {
		console.error(err);
		cAppStarted = false;
	} 
	},

	stop: function() {
		const self = this;
		self.imagehandler_cpp.childProcess.kill('SIGINT');
		self.imagehandler_cpp.end(function (err) {
		if (err){
			//throw err;
		};
		console.log('finished');
		});
		self.imagehandler_cpp.childProcess.kill('SIGKILL');
	}

});
