# Dockerfile that imports the current repo checkout build it in a builder
# container and uses cpack to get the deb to import that into the runtime container and install it
FROM debian:stable-slim AS builder

# Set working directory
WORKDIR /app

# Copy the current repo into the container
COPY . .

# Install dependencies
RUN ./scripts/setup_common_requirements.sh && apt-get install -y libpng-dev libfreetype6-dev libdbus-glib-1-dev libgtk2.0-dev curl

# Build the project
RUN mkdir build && cd build && \
    cmake -Dsvg2png_scaling=-1 -Dsvg2png_scaling_flag=-1 -Dsvg2png_scaling_nav=-1 -Dsvg2png_scaling_textures=-1 .. && \
    make maptool&& \
    cpack -G DEB

# Runtime container
FROM debian:stable-slim

# Copy the deb from the builder container
COPY --from=builder /app/build/navit.deb /tmp/navit.deb

# Install the deb package
RUN apt-get update && dpkg -I /tmp/navit.deb && apt-get install -y /tmp/navit.deb && apt-get -y install wget && rm -rf /var/lib/apt/lists/* /tmp/navit.deb

# Set the entrypoint
ENTRYPOINT ["maptool"]