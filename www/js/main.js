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
    time: {
        d: 'DAY',
        n: 'NIGHT'
    }
};

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
    const $ldrInput = $('#ldr-input');
    const $daytimeInput = $('#daytime-input');
    const $doorInput = $('#door-input');
    const $currentTimeInput = $('#current-time-input');
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
        })
        .catch(() => { console.error('LDR call failed...'); });
    })
    .then(function () {
        const ledValue = ajaxCall('digital/8');
        ledValue.then(function (response) {
            let value = JSON.parse(response).value;
            let dayValue = value === 1 ? coop.time.d : coop.time.n;
            let doorValue = value === 1 ? coop.door.o : coop.door.c;

            $daytimeInput.val(dayValue);
            $doorInput.val(doorValue);
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
    const $btnGroupBtn = $('.btn-group').find('.btn')

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
