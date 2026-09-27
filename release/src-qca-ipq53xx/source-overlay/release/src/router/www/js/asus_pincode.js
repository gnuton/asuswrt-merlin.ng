ASUS_PINCODE = {

    PincodeDivStyle: `<style>
        .popup_bg {
            position: absolute;
            top: 0;
            left: 0;
            height: 100%;
            width: 100%;
            z-index: 2000;
            backdrop-filter: blur(2px);
            -webkit-backdrop-filter: blur(2px);
        }
    
        .modal {
            position: fixed;
            top: 0;
            left: 0;
            z-index: 1060;
            display: block;
            width: 100%;
            height: 100%;
            overflow-x: hidden;
            overflow-y: auto;
            outline: 0;
        }
    
        .modal-dialog {
            position: relative;
            width: auto;
            margin: 0.5rem;
            pointer-events: none;
        }
    
        .modal-content {
            position: relative;
            display: -webkit-box;
            display: -ms-flexbox;
            display: flex;
            -webkit-box-orient: vertical;
            -webkit-box-direction: normal;
            -ms-flex-direction: column;
            flex-direction: column;
            width: 100%;
            pointer-events: auto;
            background-color: #fff;
            background-clip: padding-box;
            border: 1px solid rgba(0, 0, 0, .2);
            border-radius: 0.3rem;
            outline: 0;
            padding: 5px;
        }
    
        .modal-header {
            display: -webkit-box;
            display: -ms-flexbox;
            display: flex;
            -webkit-box-align: start;
            -ms-flex-align: start;
            align-items: flex-start;
            -webkit-box-pack: justify;
            -ms-flex-pack: justify;
            justify-content: space-between;
            padding: 1rem;
            border-top-left-radius: 0.3rem;
            border-top-right-radius: 0.3rem;
        }
    
        .modal-header .close {
            padding: 1rem;
            margin: -1rem -1rem -1rem auto;
        }
    
        .modal-title {
            color: #006ce1;
            font-weight: bold;
            font-size: 2em;
            margin: 0;
        }
    
        .modal-body {
            position: relative;
            -webkit-box-flex: 1;
            -ms-flex: 1 1 auto;
            flex: 1 1 auto;
            padding: 1rem;
        }
    
        .modal-footer {
            display: -webkit-box;
            display: -ms-flexbox;
            display: flex;
            -webkit-box-align: center;
            -ms-flex-align: center;
            align-items: center;
            justify-content: space-evenly;
            padding: 1rem;
        }
    
        .close {
            float: right;
            font-size: 1.5rem;
            font-weight: 700;
            line-height: 1;
            color: #000;
            text-shadow: 0 1px 0 #fff;
            opacity: .5;
        }
    
        .close:not(:disabled):not(.disabled) {
            cursor: pointer;
        }
    
        button.close {
            padding: 0;
            background-color: transparent;
            border: 0;
            -webkit-appearance: none;
        }
    
        .btn {
            display: inline-block;
            font-weight: 400;
            text-align: center;
            white-space: nowrap;
            vertical-align: middle;
            -webkit-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            border: 1px solid transparent;
            font-size: 1rem;
            line-height: 1.5;
            transition: color .15s ease-in-out, background-color .15s ease-in-out, border-color .15s ease-in-out, box-shadow .15s ease-in-out;
            padding: 0.5rem 1rem;
            border-radius: 0.3rem;
        }
    
        .btn:hover {
            cursor: pointer;
        }
    
        .btn.disabled, .btn:disabled {
            opacity: .65;
            cursor: not-allowed;
        }
    
        .btn-block {
            display: block;
            width: 100%;
        }
        
        .btn-circle {
            border-radius: 50%;
        }
    
        .btn-primary {
            color: #fff;
            background-color: #007bff;
            border-color: #007bff;
        }
    
        .btn-primary:hover {
            color: #fff;
            background-color: #0069d9;
            border-color: #0062cc;
        }
    
        .btn-primary.disabled, .btn-primary:disabled {
            color: #fff;
            background-color: #007bff;
            border-color: #007bff;
        }
    
        .btn-secondary {
            color: #fff;
            background-color: #6c757d;
            border-color: #6c757d;
        }
    
        .btn-secondary:hover {
            color: #fff;
            background-color: #5a6268;
            border-color: #545b62;
        }
    
        .btn-secondary.disabled, .btn-secondary:disabled {
            color: #fff;
            background-color: #6c757d;
            border-color: #6c757d;
        }
    
        .scroll-info {
            font-weight: bold;
            color: #FFC107;
            background: #fcf8ec;
            padding: 5px 10px;
            border-radius: 5px;
            margin: 1em;
            text-align: center;
        }
    
        .d-flex {
            display: flex;
        }
    
        .flex-column {
            -webkit-box-orient: vertical !important;
            -webkit-box-direction: normal !important;
            -ms-flex-direction: column !important;
            flex-direction: column !important;
        }
    
        .flex-row {
            -webkit-box-orient: horizontal !important;
            -webkit-box-direction: normal !important;
            -ms-flex-direction: row !important;
            flex-direction: row !important;
        }
    
        .gap-1 {
            gap: 1em;
        }
    
        .gap-2 {
            gap: 2em;
        }
    
        .gap-3 {
            gap: 3em;
        }
    
        .align-items-center {
            -webkit-box-align: center !important;
            -ms-flex-align: center !important;
            align-items: center !important;
        }
    
        .justify-content-center {
            -webkit-box-pack: center !important;
            -ms-flex-pack: center !important;
            justify-content: center !important;
        }
    
        .text-left {
            text-align: left !important;
        }
    
        .notice_title {
            display: flex !important;
            justify-content: center !important;
            font-size: 1.5em;
            color: #0A5EDB;
        }
        
        .pincode_show_time_selection {
            color: #000000;
        }
    
        .pincode_show_time_selection .time-text {
            font-weight: bold;
        }
    
        .pincode_show_time_selection .unit {
            font-size: 0.8em;
        }
    
        .pincode_show_time_selection input[type="radio"] {
            display: none;
        }
    
        .pincode_show_time_selection input[type="radio"] + label {
            width: 50px;
            height: 50px;
            display: flex;
            align-items: center;
            justify-content: center;
            flex-direction: column;
            border-radius: 50%;
            background-color: #ffffff;
            border: 1px solid #d2d2d2;
            line-height: normal;
            cursor: pointer;
        }
    
        .pincode_show_time_selection input[type="radio"]:checked + label {
            background-color: #007bff;
            color: #FFFFFF;
        }
    
        .pincode {
            font-size: 68px;
            font-family: fantasy;
            line-height: initial;
            border: solid 1px #989898;
            padding: 0 20px;
        }
    
        .pincode-time {
            font-size: 38px;
            font-weight: bold;
            color: #0093FF;
        }
    
        .text-white {
            color: #fff !important;
        }
    
        .gg-spinner {
            transform: scale(3)
        }
    
        .gg-spinner,
        .gg-spinner::after,
        .gg-spinner::before {
            box-sizing: border-box;
            position: relative;
            display: block;
            width: 20px;
            height: 20px
        }
    
        .gg-spinner::after,
        .gg-spinner::before {
            content: "";
            position: absolute;
            border-radius: 100px
        }
    
        .gg-spinner::before {
            animation: spinner 1s cubic-bezier(.6, 0, .4, 1) infinite;
            border: 3px solid transparent;
            border-top-color: #007bff
        }
    
        .gg-spinner::after {
            border: 3px solid;
            opacity: .2
        }
    
        @keyframes spinner {
            0% {
                transform: rotate(0deg)
            }
            to {
                transform: rotate(359deg)
            }
        }
    
        .gg-link {
            box-sizing: border-box;
            position: relative;
            display: block;
            transform: rotate(-45deg) scale(1);
            width: 8px;
            height: 2px;
            background: currentColor;
            border-radius: 4px
        }
    
        .gg-link::after,
        .gg-link::before {
            content: "";
            display: block;
            box-sizing: border-box;
            position: absolute;
            border-radius: 3px;
            width: 8px;
            height: 10px;
            border: 2px solid;
            top: -4px
        }
    
        .gg-link::before {
            border-right: 0;
            border-top-left-radius: 40px;
            border-bottom-left-radius: 40px;
            left: -6px
        }
    
        .gg-link::after {
            border-left: 0;
            border-top-right-radius: 40px;
            border-bottom-right-radius: 40px;
            right: -6px
        }
    
        .gg-mail,
        .gg-mail::after {
            display: block;
            box-sizing: border-box;
            height: 14px;
            border: 2px solid
        }
    
        .gg-mail {
            overflow: hidden;
            transform: scale(1);
            position: relative;
            width: 18px;
            border-radius: 2px
        }
    
        .gg-mail::after {
            content: "";
            position: absolute;
            border-radius: 3px;
            width: 14px;
            transform: rotate(-45deg);
            bottom: 3px;
            left: 0
        }
    
        .gg-info {
            box-sizing: border-box;
            position: relative;
            display: block;
            transform: scale(1);
            width: 20px;
            height: 20px;
            border: 2px solid;
            border-radius: 40px
        }
    
        .gg-info::after,
        .gg-info::before {
            content: "";
            display: block;
            box-sizing: border-box;
            position: absolute;
            border-radius: 3px;
            width: 2px;
            background: currentColor;
            left: 7px
        }
    
        .gg-info::after {
            bottom: 2px;
            height: 8px
        }
    
        .gg-info::before {
            height: 2px;
            top: 2px
        }
        
        .gg-chevron-down {
            box-sizing: border-box;
            position: relative;
            display: block;
            transform: scale(var(--ggs,1));
            width: 22px;
            height: 22px;
            border: 2px solid transparent;
            border-radius: 100px
        }
        
        .gg-chevron-down::after {
            content: "";
            display: block;
            box-sizing: border-box;
            position: absolute;
            width: 10px;
            height: 10px;
            border-bottom: 2px solid;
            border-right: 2px solid;
            transform: rotate(45deg);
            left: 4px;
            top: 2px
        } 
    
        .pincode-btn {
            display: flex;
            flex-direction: column;
            align-items: center;
            cursor: pointer;
        }
    
        .pincode-btn > div:first-child {
            width: 50px;
            height: 50px;
            border-radius: 50%;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        small {
            margin: 5px;
            font-size: 0.5em;
        }
        
        .text-primary {
            fill: #007bff;
        }
    
        .text-primary:hover {
            fill: #0069d9;
        }
    
        @media (min-width: 576px) {
            .modal-dialog {
                max-width: 500px;
                margin: 5rem auto;
            }
        }
      </style>`,

    ModalTheme: {
        ROG: `<style>
            .modal-content {
                background-color: #000000e6;
                border: 1px solid rgb(161, 10, 16);
            }
            .modal-title {
                color: #cf0a2c;
            }
            .btn-primary {
                background-color: #91071f;
                border: 0;
            }
            .btn-primary.disabled, .btn-primary:disabled {
                color: #fff;
                background-color: #91071f;
            }
            .btn-primary:hover {
                background-color: #cf0a2c;
            }
        </style>`,

        RT: `<style>
            .modal-content {
                background-color: #2B373B;
            }
            .modal-title {
                color: #FFFFFF;
            }
        </style>`,

        TUF: `<style>
            .modal-content {
                background-color: #000000e6;
                border: 1px solid #92650F;
            }
            .modal-title {
                color: #ffa523;
            }
            .btn-primary {
                background-color: #ffa523;
                border: 0;
            }
            .btn-primary.disabled, .btn-primary:disabled {
                color: #fff;
                background-color: #ffa523;
            }
            .btn-primary:hover {
                background-color: #D0982C;
            }
        </style>`
    },

    getTheme: () => {
        let ui_support = httpApi.hookGet("get_ui_support");

        function isSupport(_ptn) {
            return ui_support[_ptn] ? true : false;
        }

        let theme = 'UI4';
        if (isSupport("rog")) {
            return "ROG";
        } else if (isSupport("tuf")) {
            return "TUF";
        } else if (isSupport("UI4")) {
            return "";
        } else {
            return theme;
        }
    }
}

pincode_status = {
    "pincode": '',
    "duration": '',
    "end_time": '',
    "oauth_auth_status": ''
};

pincode_status.pincode = httpApi.nvramGet(['newsite_provisioning_pincode'], true).newsite_provisioning_pincode;
pincode_status.duration = httpApi.nvramGet(['newsite_provisioning_duration'], true).newsite_provisioning_duration;
pincode_status.end_time = httpApi.nvramGet(['newsite_provisioning_timestamp'], true).newsite_provisioning_timestamp;
pincode_status.oauth_auth_status = httpApi.nvramGet(['oauth_auth_status'], true).oauth_auth_status;

class PinCodeTimeSelector extends HTMLElement {
    constructor() {
        super();
        const template = document.createElement('template');
        template.innerHTML = `
            <div class="d-flex gap-2 flex-row">
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-5" name="pincode-time" value="5">
                    <label for="pincode-time-5">
                        <div class="time-text">5</div>
                        <div class="unit">min</div>
                    </label>
                </div>
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-30" name="pincode-time" value="30">
                    <label for="pincode-time-30">
                        <div class="time-text">30</div>
                        <div class="unit">min</div>
                    </label>
                </div>
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-60" name="pincode-time" value="60">
                    <label for="pincode-time-60">
                        <div class="time-text">1</div>
                        <div class="unit">hr</div>
                    </label>
                </div>
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-360" name="pincode-time" value="360">
                    <label for="pincode-time-360">
                        <div class="time-text">6</div>
                        <div class="unit">hr</div>
                    </label>
                </div>
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-720" name="pincode-time" value="720">
                    <label for="pincode-time-720">
                        <div class="time-text">12</div>
                        <div class="unit">hr</div>
                    </label>
                </div>
                <div class="pincode_show_time_selection">
                    <input type="radio" id="pincode-time-1440" name="pincode-time" value="1440">
                    <label for="pincode-time-1440">
                        <div class="time-text">24</div>
                        <div class="unit">hr</div>
                    </label>
                </div>
            </div>
        `;
        this.appendChild(template.content.cloneNode(true));
        this.div = this.querySelector('div');
        const radioElements = this.div.querySelectorAll('input[type="radio"]');
        radioElements.forEach((radio) => {
            radio.addEventListener('change', this.handleChangeTime.bind(this));
        });
    }

    selectTime = 300

    connectedCallback() {

    }

    changeTimeCallback = () => {
    }

    setChangeTimeCallback(callback) {
        if (typeof callback === "function") {
            this.changeTimeCallback = callback;
        }
    }

    handleChangeTime = (e) => {
        this.selectTime = e.target.value * 60;
        this.changeTimeCallback();
    }
}

class PinCodeRegenModel extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'});
        const template = document.createElement('template');
        template.innerHTML = `
            ${ASUS_PINCODE.PincodeDivStyle}
            <div class="popup_bg">
                <div class="modal">
                    <div class="modal-dialog">
                        <div class="modal-content">
                            <div class="modal-header">
                                <div class="modal-title">PIN code time update:</div>
                            </div>
                            <div class="modal-body">
                                <div id="pincode_time_selector"></div>
                            </div>
                            <div class="modal-footer" style="justify-content: flex-end;">
                                <button type="button" id="pincode-regen-btn" class="btn btn-primary disabled">Apply</button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        `;

        this.shadowRoot.appendChild(template.content.cloneNode(true));
        this.div = this.shadowRoot.querySelector('div');

        this.pincode_time_selector = new PinCodeTimeSelector();
        this.div.querySelector('#pincode_time_selector').appendChild(this.pincode_time_selector);
        this.pincode_time_selector.setChangeTimeCallback(() => {
            this.div.querySelector('#pincode-regen-btn').classList.remove("disabled");
        })

        this.div.querySelector('#pincode-regen-btn').addEventListener('click', this.handlePincodeRegen.bind(this));

    }

    connectedCallback() {

    }

    agree = () => {
        // this.div.remove();
        const urlencoded = new URLSearchParams();
        urlencoded.append("duration", this.pincode_time_selector.selectTime);
        const requestOptions = {
            method: 'POST',
            body: urlencoded,
        };
        fetch("get_provision_pincode.cgi", requestOptions)
            .then(response => response.json())
            .then(result => {

            })
            .catch(error => console.log('error', error));
    }

    handlePincodeRegen = (e) => {
        if (!e.target.classList.contains("disabled")) {
            if (policy_status.PP == 0 || policy_status.PP_time == "") {
                const policyModal = new PolicyModalComponent({
                    policy: "PP",
                    agreeCallback: this.agree,
                    disagreeCallback: () => {
                        alert('To use this function, please agree the privacy policy.')
                    }
                });
                policyModal.show();
            } else {
                this.agree()
            }
        }
    }

    show = () => {
        if (top.document.getElementById('policy_popup_modal') == null) {
            const policy_popup_modal = document.createElement('div');
            policy_popup_modal.id = 'policy_popup_modal';
            top.document.body.appendChild(policy_popup_modal);
            policy_popup_modal.appendChild(this);
        }
    }

}

class PinCodeDiv extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'});

        const template = document.createElement('template');
        template.innerHTML = `
            ${ASUS_PINCODE.PincodeDivStyle}
            <div class="d-flex gap-1 flex-column">
                <div id="pincode_register" class="d-flex gap-1 flex-column">
                    <div class="text-left">Please choose the PIN CODE to display the time</div>
                    <div id="pincode_time_selector"></div>
                    <div class="text-left">
                        <button id="pincode-gen-btn" class="btn btn-primary disabled">Generate PIN Code</button>
                    </div>
                </div>
                <div id="pincode_viewer" class="d-flex gap-1 flex-column text-left" style="display: none;">
                    <div class="d-flex flex-row align-items-center gap-1">
                        <div id="pincode" class="pincode"></div>
                        <div class="d-flex gap-1 flex-row">
                            <div class="pincode-btn copy-pincode"><div class="btn-primary"><i class="gg-link text-white"></i></div>Copy</div>
                            <div class="pincode-btn email-pincode"><div class="btn-primary"><i class="gg-mail text-white"></i></div>Email</div>
                        </div>
                    </div>
                    <div class="text-left">Please enter the 6-digit PIN Code on another device. This code will expire at the following time.</div>
                    <div class="d-flex flex-row align-items-center gap-1">
                        <div id="pincode_time" class="pincode-time"></div>
                    </div>
                    <div class="text-left"><button type="button" class="btn btn-primary" id="stopPincodeShare">Stop sharing PIN Code</button></div>
                </div>
                <div id="pincode_binded" class="d-flex gap-1 flex-column text-left" style="display: none;">
                    <div class="d-flex flex-row align-items-center gap-1">
                        <div>This router's control has been taken over by the IT administrator</div>
                    </div>
                </div>
                <div id="pincode_loader" class="popup_bg d-flex align-items-center justify-content-center">
                    <i class="gg-spinner"></i>
                </div>
            </div>
        `;

        this.shadowRoot.appendChild(template.content.cloneNode(true));
        this.div = this.shadowRoot.querySelector('div');

        this.pincodeViewer = this.shadowRoot.getElementById('pincode_viewer');
        this.pincodeRegister = this.shadowRoot.getElementById('pincode_register');
        this.pincodeLoader = this.shadowRoot.querySelector('#pincode_loader');
        this.pincodeBinded = this.shadowRoot.querySelector('#pincode_binded');

        this.pincode_time_selector = new PinCodeTimeSelector();
        this.pincodeRegister.querySelector('#pincode_time_selector').appendChild(this.pincode_time_selector);
        this.pincode_time_selector.setChangeTimeCallback(() => {
            this.pincodeRegister.querySelector('#pincode-gen-btn').classList.remove("disabled");
        })

        this.pincodeRegister.querySelector('#pincode-gen-btn').addEventListener('click', this.handlePincodeGen.bind(this));
        this.pincodeViewer.querySelector('#stopPincodeShare').addEventListener('click', this.handlePincodeDel.bind(this));

        if (pincode_status.oauth_auth_status == 2) {
            this.pincodeViewer.style.display = 'none';
            this.pincodeRegister.style.display = 'none';
            this.pincodeLoader.style.display = 'none';
            this.pincodeBinded.style.display = '';
        } else if (pincode_status.end_time > Math.floor(Date.now() / 1000)) {
            this.initPincodeViewer();
        } else {
            this.pincodeLoader.style.display = 'none';
        }
    }

    timeSelector = ``

    timerInterval = null

    refreshPincode = () => {
        pincode_status.pincode = httpApi.nvramGet(['newsite_provisioning_pincode'], true).newsite_provisioning_pincode;
        pincode_status.duration = httpApi.nvramGet(['newsite_provisioning_duration'], true).newsite_provisioning_duration;
        pincode_status.end_time = httpApi.nvramGet(['newsite_provisioning_timestamp'], true).newsite_provisioning_timestamp;
    }

    initPincodeRegister = () => {
        this.pincodeViewer.style.display = 'none';
        this.pincodeRegister.style.display = '';
        this.pincodeRegister.querySelector('#pincode-gen-btn').classList.add('disabled');
        const radios = this.pincodeRegister.querySelectorAll('input[type="radio"]');
        radios.forEach((radio) => {
            radio.checked = radio.defaultChecked;
        });
    }

    initPincodeViewer = () => {
        this.pincodeLoader.style.display = 'none';
        this.pincodeViewer.style.display = '';
        this.pincodeRegister.style.display = 'none';
        this.refreshPincode();
        clearInterval(this.timerInterval);
        this.pincodeViewer.querySelector('.copy-pincode').addEventListener('click', this.handleCopyPincode.bind(this));
        this.pincodeViewer.querySelector('.email-pincode').addEventListener('click', this.handleEmailPincode.bind(this));
        this.pincodeViewer.querySelector('#pincode').innerText = `${pincode_status.pincode.slice(0, 3)} ${pincode_status.pincode.slice(3)}`
        this.timerInterval = setInterval(this.updateCountdown.bind(this), 1000);
        this.updateCountdown.bind(this);
    }

    agree = () => {
        this.pincodeLoader.style.display = '';
        const urlencoded = new URLSearchParams();
        urlencoded.append("duration", this.pincode_time_selector.selectTime);
        const requestOptions = {
            method: 'POST',
            body: urlencoded,
        };
        fetch("get_provision_pincode.cgi", requestOptions)
            .then(response => response.json())
            .then(result => {
                this.initPincodeViewer()
            })
            .catch(error => console.log('error', error));
    }

    handlePincodeDel = () => {
        this.pincodeLoader.style.display = '';
        fetch("del_provision_pincode.cgi")
            .then(response => response.json())
            .then(result => {
                this.pincodeLoader.style.display = 'none';
                if (result.statusCode == 200) {
                    this.pincodeViewer.style.display = 'none';
                    this.pincodeRegister.style.display = '';
                }
            })
            .catch(error => console.log('error', error));
    }

    handlePincodeShowRegen = () => {
        const pop = new PinCodeRegenModel();
        pop.show();
    }

    handlePincodeGen = (e) => {
        if (!e.target.classList.contains("disabled")) {
            if (policy_status.PP == 0 || policy_status.PP_time == "") {
                const policyModal = new PolicyModalComponent({
                    policy: "PP",
                    agreeCallback: this.agree,
                    disagreeCallback: () => {
                        alert('To use this function, please agree the privacy policy.')
                    }
                });
                policyModal.show();
            } else {
                this.agree()
            }
        }
    }

    updateCountdown = () => {
        const currentUnixTime = Math.floor(Date.now() / 1000);
        const remainingTime = pincode_status.end_time - currentUnixTime;
        const hours = Math.floor(remainingTime / 3600);
        const minutes = Math.floor((remainingTime % 3600) / 60);
        const seconds = remainingTime % 60;
        this.shadowRoot.querySelector('#pincode_viewer').querySelector('#pincode_time').innerHTML = `${hours.toString().padStart(2, '0')}<small>h</small>:${minutes.toString().padStart(2, '0')}<small>m</small>:${seconds.toString().padStart(2, '0')}<small>s</small>`;
        if (remainingTime <= 0) {
            this.initPincodeRegister();
            clearInterval(this.timerInterval);
        }
    }

    handleCopyPincode = () => {
        if (window.isSecureContext && navigator.clipboard) {
            navigator.clipboard.writeText(text);
        } else {
            const textArea = document.createElement("textarea");
            textArea.value = pincode_status.pincode;
            document.body.appendChild(textArea);
            textArea.select();
            try {
                document.execCommand('copy')
            } catch (err) {
                console.error('Unable to copy to clipboard', err)
            }
            document.body.removeChild(textArea)
        }
    }

    handleEmailPincode = () => {

        const utcTimestamp = (pincode_status.end_time - pincode_status.duration) * 1000;
        const date = new Date(utcTimestamp);
        const year = date.getFullYear();
        const month = (date.getMonth() + 1).toString().padStart(2, '0');
        const day = date.getDate().toString().padStart(2, '0');
        const hours = date.getHours().toString().padStart(2, '0');
        const minutes = date.getMinutes().toString().padStart(2, '0');
        const timezone = `(UTC/GMT${date.getTimezoneOffset() > 0 ? `-` : `+`}${date.getTimezoneOffset() / -60})`
        const formattedDateTime = `${year}/${month}/${day} ${hours}:${minutes} ${timezone}`;
        let duation = ``;
        if (pincode_status.duration < 3600) {
            duation = `${Math.round(pincode_status.duration / 60)} minutes`
        } else {
            duation = `${Math.round(pincode_status.duration / 3600 * 100) / 100} hr`
        }
        // console.log(`mailto:?subject=[ASUS Multiple Site] PIN code invitation&body=Please enter the following PIN code, ${pincode_status.pincode}, in the Multiple Site. This PIN code will expire after ${formattedDateTime}.%0D%0A%0D%0A--%0D%0AASUSTeK Computer Inc. All rights reserved.`)
        window.location = `mailto:?subject=[ASUS Multiple Site] PIN code invitation&body=Please enter the following PIN code, ${pincode_status.pincode}, in the Multiple Site. The PIN code was generated on ${formattedDateTime}, and it will expire after ${duation}.%0D%0A%0D%0A--%0D%0AASUSTeK Computer Inc. All rights reserved.`
    }

    connectedCallback() {

    }

}

class PinCodeTitleDiv extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'});

        const template = document.createElement('template');
        template.innerHTML = `
            ${ASUS_PINCODE.PincodeDivStyle}
            <div class="d-flex align-items-center gap-1">Use the PIN code to establish a connection on ASUS Multiple Site utility <a href="#showPincodeHelp" id="showPincodeHelp" class="text-primary"><svg xmlns="http://www.w3.org/2000/svg" height="1em" viewBox="0 0 512 512"><path d="M256 8C119.043 8 8 119.083 8 256c0 136.997 111.043 248 248 248s248-111.003 248-248C504 119.083 392.957 8 256 8zm0 448c-110.532 0-200-89.431-200-200 0-110.495 89.472-200 200-200 110.491 0 200 89.471 200 200 0 110.53-89.431 200-200 200zm107.244-255.2c0 67.052-72.421 68.084-72.421 92.863V300c0 6.627-5.373 12-12 12h-45.647c-6.627 0-12-5.373-12-12v-8.659c0-35.745 27.1-50.034 47.579-61.516 17.561-9.845 28.324-16.541 28.324-29.579 0-17.246-21.999-28.693-39.784-28.693-23.189 0-33.894 10.977-48.942 29.969-4.057 5.12-11.46 6.071-16.666 2.124l-27.824-21.098c-5.107-3.872-6.251-11.066-2.644-16.363C184.846 131.491 214.94 112 261.794 112c49.071 0 101.45 38.304 101.45 88.8zM298 368c0 23.159-18.841 42-42 42s-42-18.841-42-42 18.841-42 42-42 42 18.841 42 42z"/></svg></a></div>
            <div id="pincode_modal" class="popup_bg" style="display: none">
                <div class="modal">
                    <div class="modal-dialog">
                        <div class="modal-content">
                            <div class="modal-header">
                                <h5 class="modal-title"><#NewFeatureAbout#></h5>
                                <button type="button" class="close">
                                    <span>&times;</span>
                                </button>
                            </div>
                            <div class="modal-body">
                                <div class="d-flex flex-column gap-2 text-left">
                                    <div class="d-flex flex-column">
                                        <div style="font-weight: bold">What is ASUS multiple site?</div>
                                        <div style="font-size: 0.8em; color: #919191">ASUS multiplo site is a tool for managing
                                            multiple devices. By binding dovices to the same account with ASUS multiple site, you
                                            can manage and control them remotely at the same time. Advanced features allow grouping
                                            of two or more sites, which will establish a connection through Wireguard VPN, allowing
                                            sites in the same group to communicate with each other on the same LAN.
                                        </div>
                                    </div>
                                    <div class="d-flex flex-column">
                                        <div style="font-weight: bold">How to create site in ASUS multiple site?</div>
                                        <div style="font-size: 0.8em; color: #919191">
                                            <ol style="padding-left: 0; margin: 0 0 0 13px;">
                                                <li>Using the ASUS Router APP: After entering the ASUS Router APP, click on "Settings" -> "Account Binding" to create a pin code. Enter the pin code on the ASUS multiple site within five minutes to complete the setup.</li>
                                                <li>Using web GUI: After entering the web GUI, click on "Advanced Settings" -> "Account Binding" to create a pin code. Enter the pin code on the ASUS multiple site within five minutes to complete the setup.</li>
                                            </ol>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        `;

        this.shadowRoot.appendChild(template.content.cloneNode(true));
        this.div = this.shadowRoot.querySelector('div');


        this.shadowRoot.querySelector("#showPincodeHelp").addEventListener('click', this.showPincodeHelp.bind(this));
        this.shadowRoot.querySelector('button.close').addEventListener('click', this.hidePincodeHelp.bind(this));
    }

    showPincodeHelp = () => {
        this.shadowRoot.querySelector('#pincode_modal').style.display = '';
    }

    hidePincodeHelp = () => {
        this.shadowRoot.querySelector('#pincode_modal').style.display = 'none';
    }

    connectedCallback() {

    }
}

customElements.define('pincode-div', PinCodeDiv);
customElements.define('pincode-title', PinCodeTitleDiv);
customElements.define('pincode-timeselector', PinCodeTimeSelector);
customElements.define('pincode-regen-model', PinCodeRegenModel);

class PinCodeComponent {
    constructor(props) {
        this.props = props;
        this.element = document.createElement('pincode-div');
    }

    render() {
        return this.element;
    }
}

class PinCodeTitleComponent {
    constructor(props) {
        this.props = props;
        this.element = document.createElement('pincode-title');
    }

    render() {
        return this.element;
    }
}

