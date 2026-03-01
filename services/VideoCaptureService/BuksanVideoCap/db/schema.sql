CREATE TABLE IF NOT EXISTS devices (
    deviceId BIGSERIAL PRIMARY KEY,
    type BIGINT NOT NULL,
    addDate DATE NOT NULL,
    caption TEXT NOT NULL,
    rtsp_url TEXT NOT NULL,
    assigned_node_id UUID NULL,
    status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS recordings (
    recordId BIGSERIAL PRIMARY KEY,
    "user" BIGINT NOT NULL,
    unixtime BIGINT NOT NULL,
    mediafile TEXT NOT NULL,
    alert BIGINT NULL,
    device BIGINT NOT NULL REFERENCES devices(deviceId),
    "time" TIME WITHOUT TIME ZONE NOT NULL,
    "date" DATE NOT NULL,
    mandatoryMark BIGINT NULL
);

CREATE TABLE IF NOT EXISTS nodes (
    node_id UUID PRIMARY KEY,
    caption TEXT NOT NULL,
    status TEXT NOT NULL
);
