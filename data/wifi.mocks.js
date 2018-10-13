(function() {
    window.fetch = function(path) {
        return new Promise((resolve) => {
            resolve({
                json: () => getJson(data[path])
            });
        });
    }

    function getJson(fakeData) {
        return new Promise(resolve => {
            setTimeout(() => resolve(fakeData.data), fakeData.delay || 0);
        })
    }

    const data = {
        "wifi/status": {
            data: {
                SSID: "Access Point",
                RSSI: -43,
                status: 3,
                IpAddr: "192.168.1.52"
            }
        },
        "wifi/scan": {
            delay: 1000,
            data: {
                accessPoints: [
                    { SSID: "WiFighter", RSSI: -57 },
                    { SSID: "Get your own WiFi", RSSI: -78 },
                    { SSID: "HOME13844AB", RSSI: -67 },
                    { SSID: "Neighbor", RSSI: -95 }
                ]
            }
        }
    }
    console.log("Mocks loaded.");
})();
