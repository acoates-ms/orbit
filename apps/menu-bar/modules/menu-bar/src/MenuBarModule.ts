import { NativeModule, requireNativeModule } from 'expo';
import { NativeMenuBarModule, NativeMenuBarModuleEvents } from './types';

declare class MenuBarModule extends NativeModule<NativeMenuBarModuleEvents> {}

export default requireNativeModule<MenuBarModule>('MenuBar') as MenuBarModule & NativeMenuBarModule;
