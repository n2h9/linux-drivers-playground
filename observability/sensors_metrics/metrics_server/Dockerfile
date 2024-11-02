# app/Dockerfile
FROM python:3.9-slim

# Set the working directory
WORKDIR /app

# Copy the requirements file
COPY requirements.txt .

# Install the dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy the application code
COPY server.py .
ADD bmp280 bmp280

ENTRYPOINT ["python"]
# Command to run the application
CMD ["-u", "server.py"]
