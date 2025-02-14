package hydrometer

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
			SELECT *, ROW_NUMBER() OVER (ORDER BY timestamp) AS rn,
			COUNT(*) OVER () AS total_count
			FROM readings
			WHERE plant_id = ? AND timestamp BETWEEN ? AND ?
		)
		SELECT * FROM RankedReadings
		WHERE rn % (total_count / ?) = 0 OR rn = total_count
		ORDER BY timestamp;
	`

	endTime := time.Now()

	if err := db.Raw(query, plantID, startTime, endTime, count).Scan(&readings).Error; err != nil {
		return nil, err
	}

	return readings, nil
}
