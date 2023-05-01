from flask import Flask, request
import datetime

app = Flask(__name__)

@app.route("/save-alert")
def save_alerts():
    # "MM-DD-YYYY-Alerts.txt"

    now = datetime.datetime.now()
    date_str = now.strftime("%m-%d-%Y")
    file_name = f"./data/alerts/{date_str}-Alerts.txt"
    current_time = datetime.datetime.now().strftime("%H:%M")
    with open(file_name, "a") as f:
        f.write(f"{current_time} with a duration of {request.args.get('time')} seconds\n")
    return "Alerts saved"

@app.route("/save-temperature")
def save_temperatures():
    # "MM-DD-YYYY-Temperatures.txt"

    now = datetime.datetime.now()
    date_str = now.strftime("%m-%d-%Y")
    file_name = f"./data/temperatures/{date_str}-Temperatures.txt"
    current_time = datetime.datetime.now().strftime("%H:%M")

    with open(file_name, "a") as f:
        f.write(f"{current_time} - {request.args.get('temp')} degrees Fahrenheit\n")
    return "Temperature saved"

@app.route("/daily-alert-report")
def count_alerts():
    now = datetime.datetime.now()
    date_str = now.strftime("%m-%d-%Y")
    file_name = f"./data/alerts/{date_str}-Alerts.txt"
    try:
        with open(f"{date_str}-Alerts", "r") as f:
            count = len(f.readlines())
        print(f"count is: {count}")

        if count < 5:
            return "5"
        elif count < 7:
            return "4"
        elif count < 10:
            return "3"
        elif count < 13:
            return "2"
        elif count < 16:
            return "1"
        else:
            return "0"
    except FileNotFoundError:
        return "5"
