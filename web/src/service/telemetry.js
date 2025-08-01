import { times, state, views, telemetry } from '@/service/state';

export function update_telemetry(data) {
  if (data.state) {
    Object.entries(data.state).forEach(([key, value]) => {
      const target = state.find(item => item.name === key.toUpperCase());

      if (target) {
        target.text = value;
        switch (value) {
          case "OK":
            target.status = "success";
            break;
          case "ERROR":
            target.status = "warn";
            break;
          case "FATAL":
            target.status = "danger";
            break;
          default:
            target.status = "secondary";
        }
      }
    });
  }

  if (data.gps) {

  }

  if (data.gyro) {

  }

  if (data.analog) {
    console.log(data.analog.analog);
    telemetry.analog[0].push(times.boot.raw + data.analog.timestamp / 1000);
    telemetry.analog[1].push(adc_to_v(data.analog.analog.ain1) * views.analog.ch.ain1.multiplier * (views.analog.ch.ain1.divider ? 0.5 : 1));
    telemetry.analog[2].push(adc_to_v(data.analog.analog.ain2) * views.analog.ch.ain2.multiplier * (views.analog.ch.ain2.divider ? 0.5 : 1));
    telemetry.analog[3].push(adc_to_v(data.analog.analog.ain3) * views.analog.ch.ain3.multiplier * (views.analog.ch.ain3.divider ? 0.5 : 1));
    telemetry.analog[4].push(adc_to_v(data.analog.analog.ain4) * views.analog.ch.ain4.multiplier * (views.analog.ch.ain4.divider ? 0.5 : 1));
    telemetry.analog[5].push(adc_to_v(data.analog.analog.ain5) * views.analog.ch.ain5.multiplier);
    telemetry.analog[6].push(adc_to_v(data.analog.analog.ain6) * views.analog.ch.ain6.multiplier);
    telemetry.analog[7].push(adc_to_v(data.analog.analog.voltage) * views.analog.ch.volt.multiplier);
    telemetry.analog[8].push(data.analog.analog.temperature * views.analog.ch.temp.multiplier);

    if (telemetry.chart.analog) {
      telemetry.chart.analog.setData(telemetry.analog);
      telemetry.chart.analog.setScale('x', {
        min: new Date().getTime() / 1000 - 60,
        max: new Date().getTime() / 1000
      });
    }
  }

  if (data.digital) {

  }

}

function adc_to_v(value) {
  return value / (1 << 15) * 4.096;
}
