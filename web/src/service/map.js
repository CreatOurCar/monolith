import L from 'leaflet';
import 'leaflet/dist/leaflet.css';

export function init_map(map, line, path, ref) {
    const el = ref.value?.$el || ref.value;
    if (!el) return;

    map.value = L.map(el, {
        center: [35.29194611, 126.57415666],
        zoom: 17,
        zoomControl: true
    });

    L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
        maxZoom: 19
    }).addTo(map.value);

    line.value = L.polyline([], {
        color: '#FFFF00',
        opacity: 0.9,
        weight: 3
    }).addTo(map.value);

    path.value = [];
}
