import { StyleSheet, TouchableOpacity, Platform } from 'react-native';

import { View, Text } from '../components';
import NativeColorPalette from '../components/NativeColorPalette';
import MenuBarModule from '../modules/MenuBarModule';
import { resetApolloStore, resetStorage } from '../modules/Storage';

const flex = Platform.select({windows: 0, default: 1});

const DebugMenu = () => {
  return (
    <View flex={flex} px="medium" pb="medium" padding="2" gap="1.5">
      <TouchableOpacity onPress={resetStorage}>
        <Text color="warning">Reset storage</Text>
      </TouchableOpacity>
      <TouchableOpacity onPress={resetApolloStore}>
        <Text color="warning">Clear Apollo Store</Text>
      </TouchableOpacity>
      <NativeColorPalette />
      <Text color="secondary" style={styles.about}>
        {`App version: ${MenuBarModule.appVersion}`}
      </Text>
      <Text color="secondary" style={styles.about}>
        {`Build version: ${MenuBarModule.buildVersion}`}
      </Text>
    </View>
  );
};

export default DebugMenu;

const styles = StyleSheet.create({
  about: {
    fontSize: 13,
  },
});
