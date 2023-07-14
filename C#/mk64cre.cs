if (args.Length < 2) {
	Console.WriteLine("Mario Kart 64 RAM Course Extractor\n");
	Console.WriteLine("Usage: mk64cre <input.bin> <output.obj>");
	return;
}

List<(short, short, short)> vertices = new();
List<(int, int, int)> triangles = new();

byte[] data = File.ReadAllBytes(args[0]);
Console.WriteLine($"Input file is {data.Length / 1024} KB");
Array.Resize(ref data, data.Length + 16);
for (int i = 1; i <= 16; i++)
	data[^i] = 0xFF;  // pad the file in case the vertex data table reaches end of file
int vertTable = ReadInt(data, 0x00150268);
Console.WriteLine($"Vertex       data starts at 0x{vertTable.ToString("X8")} according to pointer at 0x00150268");
int displayListTable = ReadInt(data, 0x00150274);
Console.WriteLine($"Display List data starts at 0x{displayListTable.ToString("X8")} according to pointer at 0x00150274");

// Read Vertex Table
int p;
for (p = vertTable; data[p+6] == 0 && data[p+7] >> 4 == 0; p += 16) {   //detect end of table by checking known 12 bits
	vertices.Add((
		ReadShort(data, p),
		ReadShort(data, p + 2),
		ReadShort(data, p + 4)
	));
}
Console.WriteLine($"Vertex Table ended at 0x{p.ToString("X8")}");

// Read DisplayList Table
int vertOffset = 0;
for (p = displayListTable; p < vertTable; p += 8) {
	switch (data[p]) {  // Switch on F3DEX commands
		case 0x04:  // Load Vertices
			vertOffset = ((data[p+5] << 16) | (data[p+6] << 8) | data[p+7]) >> 4;
			break;
		case 0xB1:  // Draw Two Triangles
			triangles.Add(((data[p+1] >> 1) + vertOffset, (data[p+2] >> 1) + vertOffset, (data[p+3] >> 1) + vertOffset));
			goto case 0xBF;
		case 0xBF:  // Draw One Triangle
			triangles.Add(((data[p+5] >> 1) + vertOffset, (data[p+6] >> 1) + vertOffset, (data[p+7] >> 1) + vertOffset));
			break;
	}
}

using (StreamWriter writer = new(args[1])) {
	vertices.ForEach(v => writer.WriteLine($"v {v.Item1} {v.Item2} {v.Item3}"));
	triangles.ForEach(t => writer.WriteLine($"f {t.Item1 + 1} {t.Item2 + 1} {t.Item3 + 1}"));
}
Console.WriteLine($"\nSaved {vertices.Count} vertices and {triangles.Count} triangles to {args[1]}");

static int ReadInt(byte[] data, int index) {
	return (data[index] << 24) | (data[index + 1] << 16) | (data[index + 2] << 8) | data[index + 3];
}

static short ReadShort(byte[] data, int index) {
	return (short)((data[index] << 8) | data[index + 1]);
}
