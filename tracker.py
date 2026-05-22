import sys


class PackageTracker:

    def __init__(self):
        # Configuration setup for future live API routing integration
        self.host = "://api-ninjas.com"

    def fetch_mock_tracking(self, tracking_number):
        """Simulates an API engine response for logistics tracking."""
        print(f" Fetching transit history for: {tracking_number}...\n")

        # Real-world backend apps gracefully handle structured payloads
        mock_database = {
            "CA123456789": {
                "status": "In Transit",
                "carrier": "Canada Post",
                "destination": "Montreal, QC",
                "eta": "2026-05-28",
                "history": [
                    {
                        "time": "09:30 AM",
                        "location": "Montreal Sorting Facility",
                        "detail": "Item processed",
                    },
                    {
                        "time": "04:15 AM",
                        "location": "Dorval Hub",
                        "detail": "Item arrived at facility",
                    },
                    {
                        "time": "Yesterday",
                        "location": "Toronto ON",
                        "detail": "Item picked up by carrier",
                    },
                ],
            }
        }
        return mock_database.get(tracking_number, None)

    def display_tracking(self, tracking_number):
        data = self.fetch_mock_tracking(tracking_number)

        if not data:
            print(f"Error: Tracking ID '{tracking_number}' not found.")
            return

        # Clean, professional terminal interface formatting
        print("=" * 45)
        print(f"STATUS: {data['status'].upper()} | Carrier: {data['carrier']}")
        print(f"Destination: {data['destination']} | ETA: {data['eta']}")
        print("=" * 45)
        print("TRAVELLING HISTORY:")

        for event in data["history"]:
            print(
                f" [{event['time']}] {event['location']} - {event['detail']}"
            )
        print("=" * 45)


if __name__ == "__main__":
    tracker = PackageTracker()

    # Allows terminal execution arguments: python tracker.py CA123456789
    if len(sys.argv) > 1:
        target_id = sys.argv[1]
    else:
        # Fallback default if no terminal argument is passed
        target_id = "CA123456789"

    tracker.display_tracking(target_id)
