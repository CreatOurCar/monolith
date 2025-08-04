export function init_map(map, ref) {
  window.kakao.maps.load(() => {
    map = new kakao.maps.Map(ref, {
      mapTypeId: kakao.maps.MapTypeId.HYBRID,
      // center: new kakao.maps.LatLng(37.2829317, 127.0435822),
      center: new kakao.maps.LatLng(35.2921728, 126.5740566),
      level: 3
    });
    map.addControl(new kakao.maps.MapTypeControl(), kakao.maps.ControlPosition.TOPRIGHT);
  });
}
