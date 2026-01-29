# Profile Management Implementation

## Overview

Proper profile management has been implemented in UCC, correctly interfacing with uccd's profile system.

## Architecture

### uccd Profile System
- **Data Storage**: Profiles stored as JSON with structured data
- **Profile Types**:
  - Default profiles (4 built-in): `__legacy_default__`, `__legacy_cool_and_breezy__`, `__legacy_powersave_extreme__`, `__default_custom_profile__`
  - Custom user profiles: User-created profiles with unique IDs
  
### Profile Structure
```json
{
  "id": "__default_custom_profile__",
  "name": "TUXEDO Defaults",
  "description": "Edit profile to change behaviour",
  "cpu": { ... },
  "fan": { ... },
  "display": { ... },
  "odmProfile": { ... },
  "nvidiaPowerCTRLProfile": { ... }
}
```

**Key Point**: Profile selection requires the **ID** field, not the **name** field.

## Implementation Details

### UccdClient Methods

#### 1. `getActiveProfileJSON()`
- **Purpose**: Fetch the currently active profile as JSON
- **Returns**: `std::optional<std::string>` containing profile JSON
- **DBus Method**: `GetActiveProfileJSON()`

#### 2. `setActiveProfile(profileId)`
- **Purpose**: Switch to a profile by ID
- **Parameter**: Profile ID string (e.g., `__default_custom_profile__`)
- **Returns**: `bool` indicating success
- **DBus Method**: `SetTempProfileById(string profileId)`

#### 3. `setActiveProfileByName(profileName)` ⭐ **NEW**
- **Purpose**: Switch to a profile by human-readable name
- **Parameter**: Profile name string (e.g., `TUXEDO Defaults`)
- **Returns**: `bool` indicating success
- **Process**:
  1. Calls `getProfileIdByName()` to find the profile ID
  2. Uses `setActiveProfile(id)` to switch
  3. Falls back to `SetTempProfile(name)` if ID lookup fails

#### 4. `getProfileIdByName(profileName)` ⭐ **NEW**
- **Purpose**: Find a profile's ID by its display name
- **Parameter**: Profile name string
- **Returns**: `std::optional<std::string>` containing the profile ID
- **Process**:
  1. Parses `GetProfilesJSON()` response
  2. Searches both default and custom profiles
  3. Returns the matching profile's ID

### QML Widget Integration

#### Profile Switcher Widget
Location: `ucc-widgets/profile-switcher/package/contents/ui/main.qml`

**Features Implemented**:
- ✅ Loads all profiles from uccd via `GetProfilesJSON()`
- ✅ Maintains mapping of profile names to IDs
- ✅ Displays current active profile
- ✅ Switches profiles using correct profile IDs
- ✅ Auto-updates active profile every 2 seconds
- ✅ Includes fallback for name-based switching

**Key Functions**:
```qml
function loadProfiles()
  - Fetches all profiles from uccd
  - Builds profileMap[name] = id mapping
  - Updates UI list

function setProfile(profileName)
  - Looks up profile ID from name
  - Calls SetTempProfileById with correct ID
  - Falls back to SetTempProfile if ID not found
  - Triggers update after 500ms delay

function updateActiveProfile()
  - Fetches active profile via GetActiveProfileJSON()
  - Extracts and displays profile name
```

## DBus Methods Available

### Profile Management
```
GetProfilesJSON()           → JSON array of all profiles
GetCustomProfilesJSON()     → JSON array of user profiles
GetDefaultProfilesJSON()    → JSON array of default profiles
GetActiveProfileJSON()      → JSON object of active profile
SetTempProfileById(id)      → Switch to profile by ID ✅
SetTempProfile(name)        → Switch to profile by name (fallback)
AddCustomProfile(json)      → Create new custom profile
UpdateCustomProfile(json)   → Modify existing profile
DeleteCustomProfile(id)     → Delete custom profile
CopyProfile(src, name)      → Duplicate profile
```

## Error Handling

### Proper ID-Based Switching
```cpp
// CORRECT: Use profile ID
tccdInterface.call("SetTempProfileById", ["__default_custom_profile__"])
// ✅ Returns (true,)

// INCORRECT: Use profile name (will fail in uccd)
tccdInterface.call("SetTempProfileById", ["TUXEDO Defaults"])
// ❌ Returns error, falls back to default
```

### JSON Parsing
- Profiles are returned as valid JSON arrays
- Each profile contains both `id` and `name` fields
- The `id` field is the unique identifier used for switching
- The `name` field is the user-friendly display name

## Configuration Applied

Profiles contain configuration for:
- **CPU**: Governor, frequency scaling, turbo settings, energy preferences
- **GPU**: NVIDIA power control offsets, PRIME settings
- **Fan**: Profile type (Balanced, Quiet, Silent), custom curves
- **Display**: Brightness, refresh rate, resolution
- **Webcam**: Enable/disable status
- **ODM**: Performance profiles and TDP limits
- **Charging**: Battery thresholds and charge type

## Testing

### Manual Test via DBus
```bash
# Get all profiles
gdbus call --system --dest com.uniwill.uccd \
  --object-path /com/uniwill/uccd \
  --method com.uniwill.uccd.GetProfilesJSON

# Switch to TUXEDO Defaults (using correct ID)
gdbus call --system --dest com.uniwill.uccd \
  --object-path /com/uniwill/uccd \
  --method com.uniwill.uccd.SetTempProfileById \
  "__default_custom_profile__"

# Get active profile
gdbus call --system --dest com.uniwill.uccd \
  --object-path /com/uniwill/uccd \
  --method com.uniwill.uccd.GetActiveProfileJSON
```

### Expected Output
```
Profile switched successfully → (true,)
Profile not found → Falls back to default, returns (false,)
```

## Known Profiles

| Name | ID | Type |
|------|-------|------|
| Default | `__legacy_default__` | Built-in |
| Cool and breezy | `__legacy_cool_and_breezy__` | Built-in |
| Powersave extreme | `__legacy_powersave_extreme__` | Built-in |
| TUXEDO Defaults | `__default_custom_profile__` | Built-in |
| \[Custom\] | `<unique-string>` | User-created |

## Summary

✅ **Proper Implementation**:
- Uses correct DBus methods (`SetTempProfileById` not `SetTempProfile`)
- Profile IDs instead of names for reliable switching
- Graceful fallback from name to ID lookup
- Full JSON profile parsing and display
- Real-time active profile monitoring
- Complete profile metadata handling

❌ **Previously Broken**:
- Attempting to use profile names as IDs
- No mapping between names and IDs
- No profile metadata parsing
- Hardcoded profile lists
- Error messages not indicating ID requirement

✨ **Result**: Clean, reliable profile switching with proper error handling and user-friendly interface.
