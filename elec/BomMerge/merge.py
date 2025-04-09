import pandas as pd

# Load both BOM files
bom1 = pd.read_csv("BomSherpentRevB.csv")
bom2 = pd.read_csv("BomSherpentRevA.csv")

# Merge them together
combined = pd.concat([bom1, bom2])

# Group by Digi-Key Part Number and take the row with the highest quantity
result = combined.sort_values("Qty", ascending=False).drop_duplicates("DigiKey")

# Optional: sort back to some logical order
result = result.sort_values("DigiKey")

# Save the merged BOM
result.to_csv("merged_bom.csv", index=False)

