// Disable windows linked modules for now.. until we fix them for new arch

module.exports = {
    dependencies: {
    '@react-native-clipboard/clipboard': {
        platforms: {
        windows: null,
      },
    },
    'react-native-svg': {
        platforms: {
        windows: null,
      },
    },
  },
};
