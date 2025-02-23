
-- for hydrometer readings
CREATE TABLE readings (
    id SERIAL PRIMARY KEY,
    plant_id INTEGER NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL,
    value INTEGER NOT NULL
);

CREATE TABLE clips (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMPTZ NOT NULL,
    clip BYTEA NOT NULL
);