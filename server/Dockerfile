FROM python:3.11-slim

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    HOST=0.0.0.0 \
    PORT=3000 \
    ACTIVITY_DB_PATH=/data/activity_monitor.db

WORKDIR /app

RUN pip install --no-cache-dir --upgrade pip && \
    pip install --no-cache-dir flask flask-cors flask-limiter

COPY src/ ./src/

RUN mkdir -p /data && \
    adduser --disabled-password --gecos "" appuser && \
    chown -R appuser:appuser /app /data

USER appuser

EXPOSE 3000

HEALTHCHECK --interval=30s --timeout=5s --start-period=15s --retries=3 \
    CMD python -c "import os, urllib.request; urllib.request.urlopen('http://127.0.0.1:' + os.environ.get('PORT', '3000') + '/health', timeout=4)"

CMD ["python", "-m", "src.main"]
