import sqlite3

conn = sqlite3.connect("tuning_db.db")

fetcher = conn.cursor()

fetcher.execute("SELECT * FROM problem_descriptions")

my_list = fetcher.fetchall()

ids = [x[0] for x in my_list]

for x in my_list:
  print(x[0])

