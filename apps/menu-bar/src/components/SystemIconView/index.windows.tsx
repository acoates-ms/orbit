import { StyleSheet, Text, ImageProps } from 'react-native';

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
    height: 18,
    width: 18,
  },
});

export default SystemIconView;
