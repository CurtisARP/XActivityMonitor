import flask

app = flask.Flask(__name__)

@app.route('/')
def index():
    return 'Hello, World!'

@app.route('/frames', methods=['POST'])
def frames():
    data = flask.request.json
    print(data)
    return flask.jsonify({'message': 'Frames received'}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)