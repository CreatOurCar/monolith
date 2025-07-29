import { state } from '@/service/state';

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

  }

  if (data.digital) {

  }

}
