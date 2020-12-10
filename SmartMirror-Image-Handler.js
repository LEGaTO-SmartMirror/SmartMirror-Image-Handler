/**
 * @file smartmirror-center-display.js
 *
 * @author nkucza
 * @license MIT
 *
 * @see  https://github.com/NKucza/smartmirror-center-display
 */

Module.register('SmartMirror-Image-Handler',{

	defaults: {
		image_height: 1080,
		image_width: 1920,
		rotation: 90.0,
		port: 7778,
		forgroundFPS: 30,
		backgroundFPS: 5,
		ai_art_mirror: true,	
		debug: false	
	},

	start: function() {
		self = this;
		this.is_shown = false;
		this.is_already_build = false;
		
		this.sendSocketNotification('CONFIG', this.config);	

		Log.info('Starting module: ' + this.name);

	},

	getDom: function () {

		Log.info('REFRESH DOM:  ' + this.name);
		var wrapper = document.createElement("div");
		wrapper.className = "video";

		if(this.is_shown) {
            wrapper.innerHTML = "<iframe width=\"" + this.config.image_width + "\" height=\"" + this.config.image_height + "\" src=\"http://0.0.0.0:"+ this.config.port +"\" frameborder=\"0\" allowfullscreen></iframe>";
            //wrapper.innerHTML = "<iframe width=\"" + this.config.width +"\" height=\"" + this.config.height + "\" src=\"http://0.0.0.0:5000/video_feed\" frameborder=\"0\" allowfullscreen></iframe>";
		};

		return wrapper;

	},

	suspend: function(){
		//this.sendNotification(this.config.publischerName + "SetFPS", this.config.backgroundFPS);
	},

	resume: function(){
		//this.sendNotification(this.config.publischerName + "SetFPS", this.config.forgroundFPS);
		this.is_shown = true;
		if(!this.is_already_build) {
            this.updateDom();
            this.is_already_build = true;
        };
	},


	notificationReceived: function(notification, payload) {
		const self = this;
		if(notification === 'CENTER_DISPLAY') {
			self.sendSocketNotification('CENTER_DISPLAY', payload);
		}else if (notification === 'DETECTED_GESTURES') {
			self.sendSocketNotification('DETECTED_GESTURES', payload);
		}else if (notification === 'DETECTED_FACES') {
			self.sendSocketNotification('DETECTED_FACES', payload);
		}else if (notification === 'DETECTED_OBJECTS') {
			self.sendSocketNotification('DETECTED_OBJECTS', payload);
		}else if (notification === 'RECOGNIZED_PERSONS') {
			self.sendSocketNotification('RECOGNIZED_PERSONS', payload);
		}
	},
	
	// Override socket notification handler.
	socketNotificationReceived: function(notification, payload) {
		const self = this;
		if (notification === 'IMAGE_HANDLER_FPS') {
			self.sendNotification('IMAGE_HANDLER_FPS', payload);
		} else if (notification === 'INIT') {
			//self.is_initialized = true;
			//setTimeout(() => {self.updateDom();}, 10000);
		}
	},


    getStyles: function () {
        return ['style.css'];
    }

});
