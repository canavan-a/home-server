package database

import (
	"time"

	"gorm.io/gorm"
)

type Reading struct {
	ID        uint      `gorm:"primaryKey"`
	PlantID   int       `gorm:"not null"`
	Timestamp time.Time `gorm:"not null"`
	Value     int       `gorm:"not null"`
}

func InsertReading(db *gorm.DB, reading Reading) (err error) {

	err = db.Create(&reading).Error

	return

}

func GetSpacedReadings(db *gorm.DB, plantID int, count int, startTime time.Time) ([]Reading, error) {
	var readings []Reading

	query := `
	WITH RankedReadings AS (
		SELECT *, NTILE(?) OVER (ORDER BY timestamp) AS bucket
		FROM readings
		WHERE plant_id = ?
		AND timestamp BETWEEN ? AND ?
	)
	SELECT DISTINCT ON (bucket) *
	FROM RankedReadings
	ORDER BY bucket, timestamp;
	`

	endTime := time.Now()

	if err := db.Raw(query, count, plantID, startTime, endTime).Scan(&readings).Error; err != nil {
		return nil, err
	}

	return readings, nil
}
