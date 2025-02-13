

CREATE TABLE reading (
    id SERIAL PRIMARY KEY,
    plant_id INTEGER NOT NULL,
    timestamp TIMESTAMPTZ NOT NULL,
    value INTEGER NOT NULL
);
