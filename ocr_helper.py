import aiohttp
import logging
import re
import config

logger = logging.getLogger(__name__)

async def extract_text_from_image(file_path: str) -> str:
    """
    Sends the image to ocr.space API and returns the extracted text.
    """
    if not config.OCR_SPACE_API_KEY:
        return ""
        
    url = "https://api.ocr.space/parse/image"
    payload = {
        'apikey': config.OCR_SPACE_API_KEY,
        'language': 'eng',
        'isOverlayRequired': False
    }
    
    try:
        async with aiohttp.ClientSession() as session:
            with open(file_path, 'rb') as f:
                form = aiohttp.FormData()
                form.add_field('apikey', config.OCR_SPACE_API_KEY)
                form.add_field('language', 'eng')
                form.add_field('isOverlayRequired', 'false')
                form.add_field('file', f, filename='screenshot.jpg', content_type='image/jpeg')
                
                async with session.post(url, data=form) as response:
                    res = await response.json()
                    if res.get('IsErroredOnProcessing'):
                        logger.error(f"OCR Error: {res.get('ErrorMessage')}")
                        return ""
                    
                    parsed_results = res.get('ParsedResults', [])
                    if parsed_results:
                        return parsed_results[0].get('ParsedText', '')
                    return ""
    except Exception as e:
        logger.error(f"Failed to process OCR: {e}")
        return ""

async def verify_match_result(file_path: str, p1_ign: str, p2_ign: str):
    """
    Tries to automatically determine the winner from the screenshot.
    Returns: winner_ign (str) or None if inconclusive.
    """
    text = await extract_text_from_image(file_path)
    if not text:
        return None
        
    text = text.lower()
    p1 = p1_ign.lower()
    p2 = p2_ign.lower()
    
    # Very basic parsing logic. E-football screenshots often have the score between the names.
    # A full AI model would be better, but we do basic keyword/number matching here.
    # If the user wants true AI later, they can use tgpt vision. For now, this is a placeholder structure.
    
    # Check if both player names are in the screenshot
    if p1 in text and p2 in text:
        # Example logic: Look for "3 - 1" or similar scorelines. 
        # For a robust system, this requires fine-tuning. We'll return None for now to let manual verification handle it 
        # if the text doesn't contain a clear "Victory" or clear score associated with a side.
        
        # If "victory" is found and p1 is closer to it, etc.
        if "winner" in text or "victory" in text:
            # Simplistic approach
            if text.find(p1) < text.find(p2):
                return p1_ign
            else:
                return p2_ign
                
    return None
