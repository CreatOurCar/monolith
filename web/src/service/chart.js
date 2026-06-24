import dayjs from 'dayjs/esm';
import { dark } from '@/layout/composables/layout';
import { fmt, digit } from '@/service/state';

const axis_stroke = () => (dark.value ? '#fff' : '#000');
const grid_stroke = () => (dark.value ? '#24282b' : '#ededed');

export function split_range(d_min, d_max) {
    if (d_min === d_max) {
        d_min *= 0.85;
        d_max *= 1.15;
    }

    const tick = 5;
    const step = (d_max - d_min) / (tick - 1);
    const min = Math.floor(d_min / step) * step;
    const max = min + step * tick;
    const splits = Array.from({ length: tick + 1 }, (_, i) => min + i * step);
    return { min, max, splits };
}

export function time_axis() {
    return {
        size: 35,
        values: (u, v) => v.map((x) => dayjs(x * 1000).format('HH:mm:ss')),
        stroke: axis_stroke,
        ticks: { stroke: grid_stroke },
        grid: { stroke: grid_stroke }
    };
}

// Builds uPlot scales/axes for each entry in `units`, and wires up the
// `fmt[k]` value formatters used by their series. `axis` is mutated in
// place so each scale's `range()` callback can stash its latest split
// for the matching axis's `splits()`.
export function build_unit_axes(units, axis, sizePerChar = 5) {
    const scales = {};
    const axes = [];

    for (const [i, [k, o]] of Object.entries(units).entries()) {
        axis[k] = { splits: [], min: 0, max: 0 };

        scales[k] = {
            range: (u, d_min, d_max) => {
                if (d_min === null && d_max === null) {
                    return [null, null];
                } else {
                    axis[k] = split_range(d_min, d_max);
                    return [axis[k].min, axis[k].max];
                }
            }
        };

        axes.push({
            scale: k,
            side: i % 2 ? 1 : 3,
            size: 50 + (o.unit.length - 1) * sizePerChar,
            values: (u, v) => v.map((x) => `${digit(x)}${o.unit}`),
            splits: () => axis[k].splits,
            stroke: axis_stroke,
            ticks: { stroke: grid_stroke },
            grid: { stroke: grid_stroke }
        });

        fmt[k] = (u, v, sidx, didx) => {
            const d = u.data[sidx];

            if (didx == null && d) {
                const i = d.findLastIndex((x) => x !== null);
                v = i !== -1 ? d[i] : d[d.length - 1];
            }

            if (isNaN(v) || v === null || v === undefined) {
                return '-';
            } else {
                return `${digit(v)} ${o.unit}`;
            }
        };
    }

    return { scales, axes };
}
