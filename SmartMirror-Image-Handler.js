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
		const self = this;
		this.is_initialized = false;

		self.sendSocketNotification('CONFIG', this.config);	

		Log.info('Starting module: ' + this.name);

	},

	getDom: function () {

		Log.info('REFRESH DOM:  ' + this.name);
		
			var wrapper = document.createElement("div");
			wrapper.className = "video";

			if (this.is_initialized){
			wrapper.innerHTML = "<iframe width=\"" + this.config.image_width + "\" height=\"" + this.config.image_height + "\" src=\"http://0.0.0.0:"+ this.config.port +"\" frameborder=\"0\" allowfullscreen></iframe>";
			}

			return wrapper;
		
	},


	notificationReceived: function(notification, payload) {
		if(notification === 'CENTER_DISPLAY') {
			this.sendSocketNotification('CENTER_DISPLAY', payload);
		}else if (notification === 'DETECTED_GESTURES') {
			this.sendSocketNotification('DETECTED_GESTURES', payload);
		}else if (notification === 'DETECTED_FACES') {
			this.sendSocketNotification('DETECTED_FACES', payload);
		}else if (notification === 'DETECTED_OBJECTS') {
			this.sendSocketNotification('DETECTED_OBJECTS', payload);
		}else if (notification === 'RECOGNIZED_PERSONS') {
			this.sendSocketNotification('RECOGNIZED_PERSONS', payload);
		}
	},
	
	// Override socket notification handler.
	socketNotificationReceived: function(notification, payload) {
		const self = this;
		if (notification === 'IMAGE_HANDLER_FPS') {
			this.sendNotification('IMAGE_HANDLER_FPS', payload);
		} else if (notification === 'INIT') {
			this.is_initialized = true;
			setTimeout(() => {this.updateDom();}, 1000);
		}
	},


    getStyles: function () {
        return ['style.css'];
    }

});
