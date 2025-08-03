import { convert } from '@/service/protocol';
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

  if (data.digital) {
    views.digital.ch.din1.value = data.digital.digital.din1;
    views.digital.ch.din2.value = data.digital.digital.din2;
    views.digital.ch.din3.value = data.digital.digital.din3;
    views.digital.ch.din4.value = data.digital.digital.din4;
  }

  if (data.analog) {
    telemetry.analog[0].push(times.boot.raw + data.analog.timestamp / 1000);
    telemetry.analog[1].push(convert.adc_to_v(data.analog.analog.ain1) * views.analog.ch.ain1.multiplier * (views.analog.ch.ain1.divider ? 0.5 : 1));
    telemetry.analog[2].push(convert.adc_to_v(data.analog.analog.ain2) * views.analog.ch.ain2.multiplier * (views.analog.ch.ain2.divider ? 0.5 : 1));
    telemetry.analog[3].push(convert.adc_to_v(data.analog.analog.ain3) * views.analog.ch.ain3.multiplier * (views.analog.ch.ain3.divider ? 0.5 : 1));
    telemetry.analog[4].push(convert.adc_to_v(data.analog.analog.ain4) * views.analog.ch.ain4.multiplier * (views.analog.ch.ain4.divider ? 0.5 : 1));
    telemetry.analog[5].push(convert.adc_to_v(data.analog.analog.ain5) * views.analog.ch.ain5.multiplier);
    telemetry.analog[6].push(convert.adc_to_v(data.analog.analog.ain6) * views.analog.ch.ain6.multiplier);
    telemetry.analog[7].push(convert.adc_to_v(data.analog.analog.voltage) * views.analog.ch.volt.multiplier);
    telemetry.analog[8].push(data.analog.analog.temperature * views.analog.ch.temp.multiplier);

    if (telemetry.chart.analog) {
      telemetry.chart.analog.setData(telemetry.analog);
      telemetry.chart.analog.setScale('x', {
        min: new Date().getTime() / 1000 - 60,
        max: new Date().getTime() / 1000
      });
    }
  }

  if (data.gyro) {
    telemetry.gyro[0].push(times.boot.raw + data.gyro.timestamp / 1000);
    telemetry.gyro[1].push(convert.accel_to_g(data.gyro.gyro.accel_x));
    telemetry.gyro[2].push(convert.accel_to_g(data.gyro.gyro.accel_y));
    telemetry.gyro[3].push(convert.accel_to_g(data.gyro.gyro.accel_z));
    // telemetry.gyro[4].push(data.gyro.gyro.temperature);
    telemetry.gyro[4].push(convert.gyro_to_dps(data.gyro.gyro.gyro_x));
    telemetry.gyro[5].push(convert.gyro_to_dps(data.gyro.gyro.gyro_y));
    telemetry.gyro[6].push(convert.gyro_to_dps(data.gyro.gyro.gyro_z));

    if (telemetry.chart.gyro) {
      telemetry.chart.gyro.setData(telemetry.gyro);
      telemetry.chart.gyro.setScale('x', {
        min: new Date().getTime() / 1000 - 60,
        max: new Date().getTime() / 1000
      });
    }
  }


  if (data.gps) {

  }
}
