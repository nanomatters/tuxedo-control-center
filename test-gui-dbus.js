#!/usr/bin/env node
// Simple test to verify GUI can connect to tccd DBus service

const dbus = require('dbus-next');

async function testConnection() {
    try {
        console.log('1. Connecting to system bus...');
        const bus = dbus.systemBus();
        console.log('✓ Connected to system bus');
        
        console.log('2. Getting proxy object...');
        const proxyObject = await bus.getProxyObject('com.tuxedocomputers.tccd', '/com/tuxedocomputers/tccd');
        console.log('✓ Got proxy object');
        
        console.log('3. Getting interface...');
        const iface = proxyObject.getInterface('com.tuxedocomputers.tccd');
        console.log('✓ Got interface');
        
        console.log('4. Calling GetDeviceName...');
        const deviceName = await iface.GetDeviceName();
        console.log('✓ GetDeviceName returned:', deviceName);
        
        console.log('5. Calling TuxedoWmiAvailable...');
        const wmiAvail = await iface.TuxedoWmiAvailable();
        console.log('✓ TuxedoWmiAvailable returned:', wmiAvail);
        
        console.log('6. Calling GetProfilesJSON...');
        const profiles = await iface.GetProfilesJSON();
        console.log('✓ GetProfilesJSON returned:', profiles.substring(0, 100) + '...');
        
        console.log('7. Calling GetDisplayModesJSON...');
        const displayModes = await iface.GetDisplayModesJSON();
        console.log('✓ GetDisplayModesJSON returned:', displayModes);
        
        console.log('\n✅ All tests passed! DBus connection works correctly.');
        console.log('The GUI hang is likely NOT related to tccd-ng DBus service.');
        process.exit(0);
    } catch (err) {
        console.error('\n❌ Test failed:', err.message);
        console.error('Full error:', err);
        process.exit(1);
    }
}

testConnection();
