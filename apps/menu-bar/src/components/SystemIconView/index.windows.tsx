import { StyleSheet, Text, ImageProps, PlatformColor } from 'react-native';

type Props = Omit<ImageProps, 'source'> & { systemIconName: string };

const SystemIconView = (props: Props) => {
  const { systemIconName, ...rest } = props;
  let text = '?';
  let useWebdings = false;
  if (systemIconName === 'ladybug') {
    text = '!';
    useWebdings = true;
  }
  return (
    <Text
      {...rest}
      style={[styles.icon, ...(useWebdings ? [{ fontFamily: 'Webdings' }] : []), props?.style]}>
      {systemIconName === 'ladybug' ? '!' : '?'}
    </Text>
  );
};

const styles = StyleSheet.create({
  icon: {
    color: PlatformColor('Foreground'),
    fontSize: 22,
    height: 24,
    width: 24,
  },
});

export default SystemIconView;
