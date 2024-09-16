import { TurboModuleRegistry } from 'react-native';
import type { NativeModule } from 'react-native';

type FilePickerModuleType = NativeModule & {
  pickFileWithFilenameExtension: (extensions: string[], prompt?: string) => Promise<string>;
  pickFolder: () => Promise<string>;
};

const FilePickerModule: FilePickerModuleType = TurboModuleRegistry.get('FilePicker');

export default {
  ...FilePickerModule,
  getAppAsync: async () => {
    return await FilePickerModule.pickFileWithFilenameExtension(
      ['apk', 'app', 'gzip', 'ipa', 'tar'],
      'Select'
    );
  },
  pickFolder: async () => FilePickerModule.pickFolder(),
};
