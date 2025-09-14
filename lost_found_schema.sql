 CREATE TABLE IF NOT EXISTS items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        type TEXT NOT NULL CHECK (type IN ('lost', 'found')),
        user_name TEXT NOT NULL,
        description TEXT NOT NULL,
        category TEXT NOT NULL,
        location TEXT NOT NULL,
        date_reported TEXT NOT NULL,
        contact_info TEXT,
        current_location TEXT,
        is_resolved INTEGER DEFAULT 0
    );
    CREATE TABLE IF NOT EXISTS matches (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lost_item_id INTEGER,
        found_item_id INTEGER,
        match_score INTEGER,
        status TEXT DEFAULT 'pending',
        FOREIGN KEY(lost_item_id) REFERENCES items(id),
        FOREIGN KEY(found_item_id) REFERENCES items(id)
    );