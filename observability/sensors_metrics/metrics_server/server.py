import threading
from flask import Flask
from prometheus_flask_exporter import PrometheusMetrics
from bmp280.prometheus_metrics_thread import bmp280_update_metrics_schedule_thread_func


# create Flask app
app = Flask(__name__)
metrics = PrometheusMetrics(app)

@app.route('/')
def index():
    return "silence is gold\n"

if __name__ == '__main__':
    # Create a thread to run the scheduler
    bmp280_metrics_thread = threading.Thread(target=bmp280_update_metrics_schedule_thread_func)
    bmp280_metrics_thread.start()

    app.run(host='0.0.0.0', port=8081)
