'use strict';

var Promise = require('es6-promise');

window.jQuery = require('jquery');
window.$ = require('jquery');

const rootPassword = process.env.ROOT_PASSWORD;
const timeout = 5000; // 5 seconds

require('./vendor/bootstrap');

function ajaxCall(path) {
    if (path && typeof rootPassword !== 'undefined' && rootPassword) {
        return new Promise((resolve, reject) => {
            $.ajax({
                url: 'http://' + 'root:' + rootPassword + '@coop.local/arduino/' + path,
                method: 'GET'
            })
            .done(data => resolve(data))
            .fail((response, textStatus, error) => reject(error));
        });
    }
}

function setOverride($btn, pin, value) {
    $('.btn-group').find('.btn').removeClass('active');
    $btn.addClass('active');

    ajaxCall('override/' + pin + '/' + value);
}

function resetOverride() {
    $('.btn-group').find('.btn').removeClass('active');
    ajaxCall('reset/8');
}

function loop() {
    // Inputs 
    const $ldrInput = $('#ldr-input');
    const $doorInput = $('#door-input');

    const ldrValue = ajaxCall('analog/0');
    ldrValue.then(function (response) {
        let value = JSON.parse(response).value;
        $ldrInput.val(value);
    });

    const ledValue = ajaxCall('digital/8');
    ledValue.then(function (response) {
        let value = JSON.parse(response).value;
        let doorValue = value === 1 ? 'OPEN' : 'CLOSED';

        $doorInput.val(doorValue);
    });

    setTimeout(() => loop(), timeout);
}

(function () {
    // Buttons
    const $closeBtn = $('#close-btn');
    const $openBtn = $('#open-btn');
    const $resetBtn = $('#reset-btn');

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

    setTimeout(() => loop(), timeout);
})();
