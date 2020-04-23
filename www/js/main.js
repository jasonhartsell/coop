'use strict';

var Promise = require('es6-promise');

window.jQuery = require('jquery');
window.$ = require('jquery');
require('./vendor/bootstrap');

const callTime = 10000;
const coop = {
    door: {
        o: 'OPEN',
        c: 'CLOSED'
    },
    led: {
        on: 'ON',
        off: 'OFF'
    },
    time: {
        d: 'DAY',
        n: 'NIGHT',
        t: 'TWILIGHT'
    }
};
const daytime = 300;
const nighttime = 65;

function ajaxCall(path) {
    return new Promise((resolve, reject) => {
        $.ajax({
            url: 'http://' + window.location.host + '/arduino/' + path,
            method: 'GET',
            timeout: callTime
        })
        .done(data => resolve(data))
        .fail((response, textStatus, error) => reject(error));
    });
}

function setOverride($btn, $btnGroupBtn, pin, value) {
    $btnGroupBtn.removeClass('active');
    ajaxCall('override/' + pin + '/' + value).then(function () {
        $btn.addClass('active');
    });
}

function resetOverride($btnGroupBtn) {
    $btnGroupBtn.removeClass('active');
    ajaxCall('custom/reset');
}

function loop() {
    // Inputs 
    const $currentTimeInput = $('#current-time-input');
    const $daytimeInput = $('#daytime-input');
    const $doorInput = $('#door-input');
    const $ledInput = $('#led-input');
    const $ldrInput = $('#ldr-input');
    const $previousTimeInput = $('#previous-time-input');

    const timeValues = ajaxCall('custom/time');
    timeValues.then(function (response) {
        let jsonResponse = JSON.parse(response);
        let currentTime = jsonResponse.currentTime;
        let previousTime = jsonResponse.previousTime;

        $currentTimeInput.val(currentTime);
        $previousTimeInput.val(previousTime);
    })
    .catch(() => { console.error('Time call failed...'); })
    .then(function () {
        const ldrValue = ajaxCall('analog/0');
        ldrValue.then(function (response) {
            let value = JSON.parse(response).value;
            $ldrInput.val(value);

            let timeOfDay = null;
            if (value >= daytime) {
                timeOfDay = coop.time.d; // Daytime
            } else if (value <= nighttime) {
                timeOfDay = coop.time.n; // Nighttime
            } else {
                timeOfDay = coop.time.t; // Twilight
            }

            $daytimeInput.val(timeOfDay);
        })
        .catch(() => { console.error('LDR call failed...'); });
    })
    .then(function () {
        const relayValue = ajaxCall('digital/2');
        relayValue.then(function (response) {
            let value = JSON.parse(response).value;
            let relayState = value === 1 ? coop.door.o : coop.door.c;

            $doorInput.val(relayState);
        })
        .catch(() => { console.error('Relay call failed...'); });
    })
    .then(function () {
        const ledValue = ajaxCall('digital/8');
        ledValue.then(function (response) {
            let value = JSON.parse(response).value;
            let ledState = value === 1 ? coop.led.on : coop.led.off;

            $ledInput.val(ledState);
        })
        .catch(() => { console.error('LED call failed...'); });
    })
    .then(() => loop());
}

(function () {
    // Buttons
    const $closeBtn = $('#close-btn');
    const $openBtn = $('#open-btn');
    const $resetBtn = $('#reset-btn');
    const $overridePanel = $('#overridePanel');
    const $btnGroupBtn = $('.btn-group').find('.btn');

    $closeBtn.off('click').on('click', function () {
        const $btn = $('.close-btn');
        setOverride($btn, $btnGroupBtn, 8, 0);
    });

    $openBtn.off('click').on('click', function () {
        const $btn = $('.open-btn');
        setOverride($btn, $btnGroupBtn, 8, 1);
    });

    $resetBtn.off('click').on('click', function () {
        resetOverride($btnGroupBtn);
    });

    $overridePanel.on('shown.bs.collapse', function () {
        $('html, body').animate({
            scrollTop: $overridePanel.offset().top
        }, 1000);
    });

    loop();
})();
