// update sliders based on data from Arduino
function updateSliders(jsonData) {
    console.log(jsonData);
    $("#slider-r").val(jsonData.r).slider("refresh");
    $("#slider-g").val(jsonData.g).slider("refresh");
    $("#slider-b").val(jsonData.b).slider("refresh");
}

function scheduledUpdate() {
    $.getJSON("http://sander-arduino.vonk/jsonp" + "?callback=?", null, function (jsonData) {
        updateSliders(jsonData);
    });
}

function initialize() {
    console.log("initialize");
    setInterval(scheduledUpdate, 1200);

    $("#slider-r, #slider-g, #slider-b").slider({
        stop: function (event, ui) {
            // format as JSON-P and sent to Arduino
            var pair = {};
            pair[event.target.name] = parseInt(event.target.value, 10);
            console.log(pair);
            $.getJSON("http://sander-arduino.vonk/jsonp" + "?callback=?", pair, function (jsonData) {
                updateSliders(jsonData);
            });
        }
    });
}

$(document).on("pageinit", initialize());
