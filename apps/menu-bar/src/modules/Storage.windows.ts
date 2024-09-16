import { Platform } from 'react-native';

import { apolloClient } from '../api/ApolloClient';
import { setSessionAsync } from '../commands/setSessionAsync';
// import MenuBarModule from '../modules/MenuBarModule';

export const userPreferencesStorageKey = 'user-preferences';
export const sessionSecretStorageKey = 'sessionSecret';

export type UserPreferences = {
  launchOnLogin: boolean;
  emulatorWithoutAudio: boolean;
  customSdkPath?: string;
  showIosSimulators: boolean;
  showTvosSimulators: boolean;
  showAndroidEmulators: boolean;
};

export const defaultUserPreferences: UserPreferences = {
  launchOnLogin: false,
  emulatorWithoutAudio: false,
  showIosSimulators: Platform.OS === 'macos',
  showTvosSimulators: false,
  showAndroidEmulators: true,
};

export const getUserPreferences = () => {
  const stringValue = storage.getString(userPreferencesStorageKey);
  const value = (stringValue ? JSON.parse(stringValue) : {}) as UserPreferences;
  return { ...defaultUserPreferences, ...value };
};

export const saveUserPreferences = (preferences: UserPreferences) => {
  storage.set(userPreferencesStorageKey, JSON.stringify(preferences));
};

const selectedDevicesIdsStorageKey = 'selected-devices-ids';
export type SelectedDevicesIds = {
  android?: string;
  ios?: string;
};

export const getSelectedDevicesIds = () => {
  const value = storage.getString(selectedDevicesIdsStorageKey);
  const selectedDevicesIds = (
    value
      ? JSON.parse(value)
      : {
          android: undefined,
          ios: undefined,
        }
  ) as SelectedDevicesIds;
  return selectedDevicesIds;
};

export const saveSelectedDevicesIds = (devicesIds: SelectedDevicesIds) => {
  storage.set(selectedDevicesIdsStorageKey, JSON.stringify(devicesIds));
};

export const resetStorage = () => {
  storage.clearAll();
};

/*
 Stubbing this out and replacing with dummy impl for now
export const storage = new MMKV({
  id: StorageUtils.MMKVInstanceId,
  path:
    Platform.OS !== 'web' ? StorageUtils.getExpoOrbitDirectory(MenuBarModule.homedir) : undefined,
});
*/
let _localStorage: { [keyof: string]: boolean | string | number | Uint8Array } = {};
interface Listener {
  onValueChanged: (key: string) => void;
  id: number;
  remove: () => void;
}
let _localStorageListeners: Listener[] = [];
let _nextId = 1;
export const storage = {
  /**
   * Set a value for the given `key`.
   */
  set: (key: string, value: boolean | string | number | Uint8Array) => {
    _localStorage[key] = value;
    _localStorageListeners.forEach((listener) => {
      listener.onValueChanged(key);
    });
  },
  /**
   * Get the boolean value for the given `key`, or `undefined` if it does not exist.
   *
   * @default undefined
   */
  getBoolean: (key: string) => {
    return _localStorage[key] as boolean;
  },
  /**
   * Get the string value for the given `key`, or `undefined` if it does not exist.
   *
   * @default undefined
   */
  getString: (key: string) => {
    return _localStorage[key] as string;
  },
  /**
   * Get the number value for the given `key`, or `undefined` if it does not exist.
   *
   * @default undefined
   */
  //_getNumber: (key: string) => number | undefined;
  /**
   * Checks whether the given `key` is being stored in this MMKV instance.
   */
  _contains: (key: string) => {
    return !!_localStorage[key];
  },
  clearAll: () => {
    const keys = Object.keys(_localStorage);
    _localStorage = {};
    keys.forEach((key) => {
      _localStorageListeners.forEach((listener) => {
        listener.onValueChanged(key);
      });
    });
  },
  /**
   * Delete the given `key`.
   */
  delete: (key: string) => {
    delete _localStorage[key];
    _localStorageListeners.forEach((listener) => {
      listener.onValueChanged(key);
    });
  },

  addOnValueChangedListener: (onValueChanged: (key: string) => void) => {
    let id = _nextId++;
    let listener: Listener = {
      id,
      onValueChanged,
      remove: () => {
        let index = _localStorageListeners.findIndex((_) => _.id === id);
        _localStorageListeners.splice(index, 1);
      },
    };
    _localStorageListeners.push(listener);
    return listener;
  },
};

const hasSetSessionFile = 'hasSetSessionFile';
if (!storage.getBoolean(hasSetSessionFile) && Platform.OS !== 'web') {
  setSessionAsync(storage.getString(sessionSecretStorageKey) ?? '');
  storage.set(hasSetSessionFile, true);
}

export function saveSessionSecret(sessionSecret: string | undefined) {
  if (sessionSecret === undefined) {
    storage.delete(sessionSecretStorageKey);
  } else {
    storage.set(sessionSecretStorageKey, sessionSecret);
  }
  setSessionAsync(sessionSecret ?? '');
}

export function resetApolloStore() {
  apolloClient.resetStore();
  storage.delete('apollo-cache-persist');
}
