export function init_map(map, line, path, ref) {
  window.kakao.maps.load(() => {
    map.value = new kakao.maps.Map(ref.value, {
      mapTypeId: kakao.maps.MapTypeId.HYBRID,
      // center: new kakao.maps.LatLng(37.2829317, 127.0435822),
      center: new kakao.maps.LatLng(35.29194611, 126.57415666),
      level: 2
    });
    map.value.addControl(new kakao.maps.MapTypeControl(), kakao.maps.ControlPosition.TOPRIGHT);

    line.value = new kakao.maps.Polyline({
      strokeColor: '#FFFF00',
      strokeOpacity: 0.9,
    });

    line.value.setPath(path.value);
    line.value.setMap(map.value);
  });
}
