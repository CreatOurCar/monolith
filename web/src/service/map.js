import { line, path } from '@/service/telemetry';

export function init_map(map, ref) {
  window.kakao.maps.load(() => {
    map.value = new kakao.maps.Map(ref.value, {
      mapTypeId: kakao.maps.MapTypeId.HYBRID,
      // center: new kakao.maps.LatLng(37.2829317, 127.0435822),
      center: new kakao.maps.LatLng(35.29194611, 126.57415666),
      level: 2
    });
    map.value.addControl(new kakao.maps.MapTypeControl(), kakao.maps.ControlPosition.TOPRIGHT);

    line.value = new kakao.maps.Polyline({
      strokeColor: '#FF00FF',
      strokeOpacity: 0.8,
    });

    line.value.setPath(path.value);
    line.value.setMap(map.value);
  });
}
