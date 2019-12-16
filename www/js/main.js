'use strict';

var Promise = require('es6-promise');

window.jQuery = require('jquery');
window.$ = require('jquery');

const rootPassword = process.env.ROOT_PASSWORD;
const timeout = 10000; // 10 seconds
const daytime = 300;

require('./vendor/bootstrap');

function ajaxCall(path) {
    if (path && typeof rootPassword !== 'undefined' && rootPassword) {
        return new Promise((resolve, reject) => {
            $.ajax({
                url: 'http://' + 'root:' + rootPassword + '@coop.local/arduino/' + path,
                method: 'GET',
                timeout: 5000
            })
            .done(data => resolve(data))
            .fail((response, textStatus, error) => reject(error));
        });
    }
}

function setOverride($btn, pin, value) {
    $('.btn-group').find('.btn').removeClass('active');

    ajaxCall('override/' + pin + '/' + value).then(function () {
        $btn.addClass('active');
    });
}

function resetOverride() {
    $('.btn-group').find('.btn').removeClass('active');
    ajaxCall('custom/reset');
}

function loop() {
    // Inputs 
    const $ldrInput = $('#ldr-input');
    const $daytimeInput = $('#daytime-input');
    const $doorInput = $('#door-input');
    const $currentTimeInput = $('#current-time-input');
    const $previousTimeInput = $('#previous-time-input');

    const ldrValue = ajaxCall('analog/0');
    ldrValue.then(function (response) {
        let value = JSON.parse(response).value;
        let dayValue = parseInt(value);

        if (dayValue >= daytime) {
            $daytimeInput.val('DAY');
        } else {
            $daytimeInput.val('NIGHT');
        }   

        $ldrInput.val(value);
    })
    .then(function () {
        const ledValue = ajaxCall('digital/8');
        ledValue.then(function (response) {
            let value = JSON.parse(response).value;
            let doorValue = value === 1 ? 'OPEN' : 'CLOSED';
    
            $doorInput.val(doorValue);
        });
    })
    .then(function () {
        const timeValues = ajaxCall('custom/time');
        timeValues.then(function (response) {
            let jsonResponse = JSON.parse(response);
            let currentTime = jsonResponse.currentTime;
            let previousTime = jsonResponse.previousTime;

            $currentTimeInput.val(currentTime);
            $previousTimeInput.val(previousTime);
        });
    }).then(function () {
        setTimeout(() => loop(), timeout);
    });
}

(function () {
    // Buttons
    const $closeBtn = $('#close-btn');
    const $openBtn = $('#open-btn');
    const $resetBtn = $('#reset-btn');
    const $overridePanel = $('#overridePanel');

    $closeBtn.off('click').on('click', function () {
        const $btn = $('.close-btn');
        setOverride($btn, 8, 0);
    });

    $openBtn.off('click').on('click', function () {
        const $btn = $('.open-btn');
        setOverride($btn, 8, 1);
    });

    $resetBtn.off('click').on('click', function () {
        resetOverride();
    });
    
    $overridePanel.on('shown.bs.collapse', function () {
        $('html, body').animate({
           scrollTop: $overridePanel.offset().top
        }, 1000);
    });

    loop();
})();
